#include "eval.h"

#include <stdlib.h>
#include "parser.h"
#include "gc.h"
#include "objects/class.h"
#include "objects/function.h"
#include "objects/str.h"
#include "objects/instance_method.h"

#include "objects/attribute_error.h"
#include "objects/exception.h"
#include "objects/syntax_error_ex.h"
#include "objects/type_error.h"
#include "objects/not_implemented_error.h"


typedef wsky_Object Object;
typedef wsky_Class Class;
typedef wsky_ASTNode Node;
typedef wsky_Scope Scope;
typedef wsky_ReturnValue ReturnValue;
typedef wsky_Value Value;
typedef wsky_LiteralNode LiteralNode;


#define TO_LITERAL_NODE(n) ((LiteralNode *) (n))

#define IS_BOOL(value) ((value).type == wsky_Type_BOOL)
#define IS_INT(value) ((value).type == wsky_Type_INT)
#define IS_FLOAT(value) ((value).type == wsky_Type_FLOAT)


static ReturnValue createUnsupportedBinOpError(const char *leftClass,
                                               const char *operator,
                                               Value right) {
  char message[128];
  snprintf(message, 127,
           "Unsupported classes for %s: %s and %s",
           operator,
           leftClass,
           wsky_getClassName(right));

  wsky_RETURN_NEW_TYPE_ERROR(message);
}

static ReturnValue createUnsupportedUnaryOpError(const char *operator,
                                                 const char *rightClass) {
  char message[128];
  snprintf(message, 127,
           "Unsupported class for unary %s: %s",
           operator,
           rightClass);

  wsky_RETURN_NEW_TYPE_ERROR(message);
}


/*
 * Returns a new `NotImplementedException`
 */
#define RETURN_NOT_IMPL(operator)                               \
  wsky_RETURN_EXCEPTION(wsky_NotImplementedError_new(operator))


/*
 * Returns true if the given object is not null and is a
 * `NotImplementedException`
 */
#define IS_NOT_IMPLEMENTED_ERROR(e)                     \
  ((e) && (e)->class == wsky_NotImplementedError_CLASS)


#include "eval_int.c"
#include "eval_float.c"
#include "eval_bool.c"


static const char *getBinOperatorMethodName(wsky_Operator operator,
                                            bool right) {
  static char buffer[64];
  snprintf(buffer, 63, "operator %s%s",
           right ? "r" : "",
           wsky_Operator_toString(operator));
  return buffer;
}

static ReturnValue evalBinOperatorValues(Value left,
                                         wsky_Operator operator,
                                         Value right,
                                         bool reverse) {
  switch (left.type) {
  case wsky_Type_BOOL:
    return evalBinOperatorBool(left.v.boolValue, operator, right);

  case wsky_Type_INT:
    return evalBinOperatorInt(left.v.intValue, operator, right);

  case wsky_Type_FLOAT:
    return evalBinOperatorFloat(left.v.floatValue, operator, right);

  case wsky_Type_OBJECT: {
    const char *method = getBinOperatorMethodName(operator, reverse);
    Object *object = left.v.objectValue;
    return wsky_Object_callMethod1(object, method, right);
  }
  }
}


static ReturnValue evalUnaryOperatorValues(wsky_Operator operator,
                                           Value right) {

  switch (right.type) {
  case wsky_Type_BOOL:
    return evalUnaryOperatorBool(operator, right.v.boolValue);

  case wsky_Type_INT:
    return evalUnaryOperatorInt(operator, right.v.intValue);

  case wsky_Type_FLOAT:
    return evalUnaryOperatorFloat(operator, right.v.floatValue);

  default:
    return createUnsupportedUnaryOpError(wsky_Operator_toString(operator),
                                         wsky_getClassName(right));
  }
}

static ReturnValue evalBinOperator(const Node *leftNode,
                                   wsky_Operator operator,
                                   const Node *rightNode,
                                   Scope *scope) {
  ReturnValue leftRV = wsky_evalNode(leftNode, scope);
  if (leftRV.exception) {
    return leftRV;
  }
  ReturnValue rightRV = wsky_evalNode(rightNode, scope);
  if (rightRV.exception) {
    return rightRV;
  }

  ReturnValue rv = evalBinOperatorValues(leftRV.v, operator, rightRV.v,
                                         false);
  if (!IS_NOT_IMPLEMENTED_ERROR(rv.exception)) {
    return rv;
  }

  ReturnValue rev;
  rev = evalBinOperatorValues(rightRV.v, operator, leftRV.v, true);
  if (!IS_NOT_IMPLEMENTED_ERROR(rev.exception)) {
    if (rev.exception) {
      printf("! %s\n", rev.exception->class->name);
    }
    return rev;
  }

  rev = evalBinOperatorValues(rightRV.v, operator, leftRV.v, false);
  if (!IS_NOT_IMPLEMENTED_ERROR(rev.exception)) {
    return rev;
  }

  return createUnsupportedBinOpError(wsky_getClassName(leftRV.v),
                                     wsky_Operator_toString(operator),
                                     rightRV.v);
}


static ReturnValue evalUnaryOperator(wsky_Operator operator,
                                     const Node *rightNode,
                                     Scope *scope) {
  ReturnValue rightRV = wsky_evalNode(rightNode, scope);
  if (rightRV.exception) {
    return rightRV;
  }
  return evalUnaryOperatorValues(operator, rightRV.v);
}

static ReturnValue evalOperator(const wsky_OperatorNode *n, Scope *scope) {
  wsky_Operator op = n->operator;
  Node *leftNode = n->left;
  Node *rightNode = n->right;
  if (leftNode) {
    return evalBinOperator(leftNode, op, rightNode, scope);
  }
  return evalUnaryOperator(op, rightNode, scope);
}


static ReturnValue evalSequence(const wsky_SequenceNode *n,
                                Scope *parentScope) {
  wsky_ASTNodeList *child = n->children;
  ReturnValue last = wsky_ReturnValue_NULL;
  Scope *scope = wsky_Scope_new(parentScope, NULL);
  while (child) {
    last = wsky_evalNode(child->node, scope);
    child = child->next;
  }
  return last;
}

static ReturnValue evalVar(const wsky_VarNode *n, Scope *scope) {
  if (wsky_Scope_containsVariableLocally(scope, n->name)) {
    wsky_RETURN_NEW_EXCEPTION("Identifier already declared");
  }
  Value value = wsky_Value_NULL;
  if (n->right) {
    ReturnValue rv = wsky_evalNode(n->right, scope);
    if (rv.exception) {
      return rv;
    }
    value = rv.v;
  }
  wsky_Scope_addVariable(scope, n->name, value);
  wsky_RETURN_VALUE(value);
}


static ReturnValue evalIdentifier(const wsky_IdentifierNode *n,
                                  Scope *scope) {
  const char *name = n->name;
  if (!wsky_Scope_containsVariable(scope, name)) {
    wsky_RETURN_NEW_EXCEPTION("Use of undeclared identifier");
  }
  wsky_RETURN_VALUE(wsky_Scope_getVariable(scope, name));
}


static ReturnValue evalAssignement(const wsky_AssignmentNode *n,
                                   Scope *scope) {
  const wsky_IdentifierNode *leftNode = (wsky_IdentifierNode *) n->left;
  if (leftNode->type != wsky_ASTNodeType_IDENTIFIER)
    wsky_RETURN_NEW_EXCEPTION("Not assignable expression");
  if (!wsky_Scope_containsVariable(scope, leftNode->name)) {
    wsky_RETURN_NEW_EXCEPTION("Use of undeclared identifier");
  }
  ReturnValue right = wsky_evalNode(n->right, scope);
  if (right.exception)
    return right;
  wsky_Scope_setVariable(scope, leftNode->name, right.v);
  return right;
}


static ReturnValue evalFunction(const wsky_FunctionNode *n,
                                Scope *scope) {
  wsky_Function *function = wsky_Function_new("<function>", n, scope);
  wsky_RETURN_OBJECT((wsky_Object *) function);
}


static Value *evalParameters(const wsky_ASTNodeList *nodes,
                             wsky_Exception **exceptionPointer,
                             Scope *scope) {
  unsigned paramCount = wsky_ASTNodeList_getCount(nodes);
  Value *values = wsky_MALLOC(sizeof(Value) * paramCount);
  unsigned i;
  for (i = 0; i < paramCount; i++) {
    ReturnValue rv = wsky_evalNode(nodes->node, scope);
    if (rv.exception) {
      *exceptionPointer = rv.exception;
      wsky_FREE(values);
      return NULL;
    }
    values[i] = rv.v;
    nodes = nodes->next;
  }
  return values;
}


/*
static bool isCallable(const Value value) {
  return wsky_isFunction(value) || wsky_isInstanceMethod(value);
}
*/

static ReturnValue callMethod(Object *instanceMethod_,
                              Value *parameters,
                              unsigned parameterCount) {
  wsky_InstanceMethod *instanceMethod;
  instanceMethod = (wsky_InstanceMethod *) instanceMethod_;
  wsky_MethodObject *method = instanceMethod->method;

  Value *self = &instanceMethod->self;

  if (self->type == wsky_Type_OBJECT && self->v.objectValue) {
    return wsky_MethodObject_call(method,
                                  self->v.objectValue,
                                  parameterCount,
                                  parameters);
  }

  return wsky_MethodObject_callValue(method,
                                     *self,
                                     parameterCount,
                                     parameters);
}

static ReturnValue callFunction(wsky_Function *function,
                                Value *parameters,
                                unsigned parameterCount) {
  return wsky_Function_call(function, NULL, parameterCount, parameters);
}

static ReturnValue evalCall(const wsky_CallNode *callNode, Scope *scope) {
  ReturnValue rv = wsky_evalNode(callNode->left, scope);
  if (rv.exception)
    return rv;

  wsky_Exception *exception;
  Value *parameters = evalParameters(callNode->children, &exception, scope);
  if (!parameters) {
    wsky_RETURN_EXCEPTION(exception);
  }
  unsigned paramCount = wsky_ASTNodeList_getCount(callNode->children);

  if (rv.v.type != wsky_Type_OBJECT) {
    wsky_RETURN_NEW_EXCEPTION("Only methods and functions are callable");
  }
  if (wsky_isFunction(rv.v)) {
    Object *function = rv.v.v.objectValue;
    rv = callFunction((wsky_Function *) function, parameters, paramCount);

  } else if (wsky_isInstanceMethod(rv.v)) {
    Object *instMethod = rv.v.v.objectValue;
    rv = callMethod(instMethod, parameters, paramCount);

  } else {
    rv = wsky_ReturnValue_newException("Only methods and "
                                       "functions are callable");
  }

  wsky_FREE(parameters);
  return rv;
}


static ReturnValue evalMemberAccess(const wsky_MemberAccessNode *dotNode,
                                    Scope *scope) {
  ReturnValue rv = wsky_evalNode(dotNode->left, scope);
  if (rv.exception)
    return rv;
  wsky_Class *class = wsky_getClass(rv.v);
  wsky_MethodObject *method = wsky_Class_findMethod(class, dotNode->name);
  if (!method) {
    char buffer[128];
    snprintf(buffer, 127, "%s object has no attribute %s",
             class->name, dotNode->name);
    wsky_RETURN_NEW_ATTRIBUTE_ERROR(buffer);
  }

  Value self = rv.v;
  if (method->flags & wsky_MethodFlags_GET) {
    if (self.type == wsky_Type_OBJECT && self.v.objectValue)
      return wsky_MethodObject_call0(method, self.v.objectValue);
    else
      return wsky_MethodObject_callValue0(method, self);
  }
  wsky_InstanceMethod *instMethod;
  instMethod = wsky_InstanceMethod_new(method, &self);
  wsky_RETURN_OBJECT((Object *) instMethod);
}



ReturnValue wsky_evalNode(const Node *node, Scope *scope) {
#define CASE(type) case wsky_ASTNodeType_ ## type
  switch (node->type) {

  CASE(NULL):
    return wsky_ReturnValue_NULL;

  CASE(BOOL):
    wsky_RETURN_BOOL(TO_LITERAL_NODE(node)->v.boolValue);

  CASE(INT):
    wsky_RETURN_INT(TO_LITERAL_NODE(node)->v.intValue);

  CASE(FLOAT):
    wsky_RETURN_FLOAT(TO_LITERAL_NODE(node)->v.floatValue);

  CASE(SEQUENCE):
    return evalSequence((const wsky_SequenceNode *) node, scope);

  CASE(STRING):
    wsky_RETURN_CSTRING(TO_LITERAL_NODE(node)->v.stringValue);

  CASE(UNARY_OPERATOR):
  CASE(BINARY_OPERATOR):
    return evalOperator((const wsky_OperatorNode *) node, scope);

  CASE(VAR):
    return evalVar((const wsky_VarNode *) node, scope);

  CASE(IDENTIFIER):
    return evalIdentifier((const wsky_IdentifierNode *) node, scope);

  CASE(ASSIGNMENT):
    return evalAssignement((const wsky_AssignmentNode *) node, scope);

  CASE(FUNCTION):
    return evalFunction((const wsky_FunctionNode *) node, scope);

  CASE(CALL):
    return evalCall((const wsky_CallNode *) node, scope);

  CASE(MEMBER_ACCESS):
    return evalMemberAccess((const wsky_MemberAccessNode *) node, scope);

  default:
    fprintf(stderr,
            "wsky_evalNode(): Unsupported node type %d\n",
            node->type);
    abort();
  }
#undef CASE
}


wsky_ReturnValue wsky_evalString(const char *source) {
  wsky_ParserResult pr = wsky_parseString(source);
  if (!pr.success) {
    char *msg = wsky_SyntaxError_toString(&pr.syntaxError);
    wsky_SyntaxErrorEx *e = wsky_SyntaxErrorEx_new(&pr.syntaxError);
    wsky_SyntaxError_free(&pr.syntaxError);
    wsky_FREE(msg);
    wsky_RETURN_EXCEPTION(e);
  }
  Scope *scope = wsky_Scope_new(NULL, NULL);
  ReturnValue v = wsky_evalNode(pr.node, scope);
  wsky_ASTNode_delete(pr.node);

  wsky_GC_unmarkAll();
  wsky_GC_visitBuiltins();
  if (v.exception)
    wsky_GC_VISIT(v.exception);
  else
    wsky_GC_VISIT_VALUE(v.v);
  wsky_GC_collect();

  return v;
}
