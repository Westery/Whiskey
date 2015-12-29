#ifndef AST_H_
# define AST_H_

/**
 * \defgroup ast Abstract Syntax Tree
 *
 * \addtogroup ast
 * @{
 *
 */

# include "position.h"
# include "token.h"

/**
 * An enumeration of the types of the nodes
 */
typedef enum {
  /* Templates only */
  wsky_ASTNodeType_HTML,
  wsky_ASTNodeType_TPLT_PRINT,

  /* Literals */
  wsky_ASTNodeType_NULL,
  wsky_ASTNodeType_BOOL,
  wsky_ASTNodeType_INT,
  wsky_ASTNodeType_FLOAT,
  wsky_ASTNodeType_STRING,
  wsky_ASTNodeType_ARRAY,

  wsky_ASTNodeType_IDENTIFIER,

  /* Variable declaration */
  wsky_ASTNodeType_VAR,

  wsky_ASTNodeType_ASSIGNMENT,

  /* Parentheses */
  wsky_ASTNodeType_SEQUENCE,

  /* Function definition */
  wsky_ASTNodeType_FUNCTION,

  /* Function call */
  wsky_ASTNodeType_CALL,

  wsky_ASTNodeType_UNARY_OPERATOR,
  wsky_ASTNodeType_BINARY_OPERATOR,

  wsky_ASTNodeType_MEMBER_ACCESS,

} wsky_ASTNodeType;



# define wsky_ASTNode_HEAD                      \
  wsky_ASTNodeType type;                        \
  wsky_Position position;


/**
 * A node of the Abstract Syntax Tree.
 */
typedef struct {
  wsky_ASTNode_HEAD
} wsky_ASTNode;



wsky_ASTNode *wsky_ASTNode_copy(const wsky_ASTNode *source);

bool wsky_ASTNode_isAssignable(const wsky_ASTNode *node);

/**
 * Returns a malloc'd string.
 */
char *wsky_ASTNode_toString(const wsky_ASTNode *node);

/**
 * Prints the node to the given file.
 */
void wsky_ASTNode_print(const wsky_ASTNode *node, FILE *output);

/**
 * Deletes the node and its children.
 */
void wsky_ASTNode_delete(wsky_ASTNode *node);



typedef struct {
  wsky_ASTNode_HEAD

  union {
    int64_t intValue;
    double floatValue;
    char *stringValue;
    bool boolValue;
  } v;
} wsky_LiteralNode;

wsky_LiteralNode *wsky_LiteralNode_new(const wsky_Token *token);



typedef struct {
  wsky_ASTNode_HEAD

  char *name;
} wsky_IdentifierNode;

wsky_IdentifierNode *wsky_IdentifierNode_new(const wsky_Token *token);



typedef struct {
  wsky_ASTNode_HEAD
  char *content;
} wsky_HtmlNode;

wsky_HtmlNode *wsky_HtmlNode_new(const wsky_Token *token);



typedef struct {
  wsky_ASTNode_HEAD
  wsky_ASTNode *child;
} wsky_TpltPrintNode;

wsky_TpltPrintNode *wsky_TpltPrintNode_new(const wsky_Token *token,
                                           wsky_ASTNode *child);



typedef struct {
  wsky_ASTNode_HEAD

  /* NULL if unary operator */
  wsky_ASTNode *left;

  wsky_Operator operator;

  wsky_ASTNode *right;

} wsky_OperatorNode;

wsky_OperatorNode *wsky_OperatorNode_new(const wsky_Token *token,
                                         wsky_ASTNode *left,
                                         wsky_Operator operator,
                                         wsky_ASTNode *right);

wsky_OperatorNode *wsky_OperatorNode_newUnary(const wsky_Token *token,
                                              wsky_Operator operator,
                                              wsky_ASTNode *right);



/* A linked list of ASTNode */
typedef struct wsky_ASTNodeList_s wsky_ASTNodeList;

struct wsky_ASTNodeList_s {
  wsky_ASTNode *node;
  wsky_ASTNodeList *next;
};

wsky_ASTNodeList *wsky_ASTNodeList_new(wsky_ASTNode *node,
                                       wsky_ASTNodeList *next);

/* Deep copy */
wsky_ASTNodeList *wsky_ASTNodeList_copy(const wsky_ASTNodeList *source);

/* Returns the last node or NULL if the list is empty */
wsky_ASTNodeList *wsky_ASTNodeList_getLast(wsky_ASTNodeList *list);

/* Returns the last node or NULL if the list is empty */
wsky_ASTNode *wsky_ASTNodeList_getLastNode(wsky_ASTNodeList *list);

void wsky_ASTNodeList_add(wsky_ASTNodeList **listPointer,
                          wsky_ASTNodeList *new);

void wsky_ASTNodeList_addNode(wsky_ASTNodeList **listPointer,
                              wsky_ASTNode *node);

void wsky_ASTNodeList_delete(wsky_ASTNodeList *list);

unsigned wsky_ASTNodeList_getCount(const wsky_ASTNodeList *list);

/* Returns a malloc'd string. */
char *wsky_ASTNodeList_toString(wsky_ASTNodeList *list,
                                const char *separator);



# define wsky_ListNode_HEAD                     \
  wsky_ASTNode_HEAD                             \
  wsky_ASTNodeList *children;

typedef struct {
  wsky_ListNode_HEAD

  bool program;
} wsky_SequenceNode;



wsky_SequenceNode *wsky_SequenceNode_new(const wsky_Position *position,
                                         wsky_ASTNodeList *children);



typedef struct {
  wsky_ListNode_HEAD

  wsky_ASTNodeList *parameters;

} wsky_FunctionNode;

wsky_FunctionNode *wsky_FunctionNode_new(const wsky_Token *token,
                                         wsky_ASTNodeList *parameters,
                                         wsky_ASTNodeList *children);



typedef struct {
  wsky_ASTNode_HEAD

  char *name;

  /* The right node or NULL */
  wsky_ASTNode *right;
} wsky_VarNode;

wsky_VarNode *wsky_VarNode_new(const wsky_Token *token,
                               const char *name,
                               wsky_ASTNode *right);



typedef struct {
  wsky_ASTNode_HEAD
  wsky_ASTNode *left;
  wsky_ASTNode *right;
} wsky_AssignmentNode;

wsky_AssignmentNode *wsky_AssignmentNode_new(const wsky_Token *token,
                                             wsky_ASTNode *left,
                                             wsky_ASTNode *right);



typedef struct {
  wsky_ListNode_HEAD
  wsky_ASTNode *left;
} wsky_CallNode;

wsky_CallNode *wsky_CallNode_new(const wsky_Token *token,
                                 wsky_ASTNode *left,
                                 wsky_ASTNodeList *children);



typedef struct {
  wsky_ASTNode_HEAD
  wsky_ASTNode *left;
  char *name;
} wsky_MemberAccessNode;

wsky_MemberAccessNode *wsky_MemberAccessNode_new(const wsky_Token *token,
                                                 wsky_ASTNode *left,
                                                 const char *name);

/**
 * @}
 */

#endif /* !AST_H_ */
