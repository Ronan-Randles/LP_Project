/*--------------------------------------------------------------------------*/
/*                                                                          */
/*       parser2                                                            */
/*                                                                          */
/*                                                                          */
/*       Group Members:          ID numbers                                 */
/*                                                                          */
/*       Ronan Randles            19242441                                  */
/*                                                                          */
/*--------------------------------------------------------------------------*/
/*      parser2 builds on parse1 with the addition of augmented s-algol     */
/*      error recovery                                                    */
/*--------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "headers/global.h" /* "header/ can be removed later" */
#include "headers/scanner.h"
#include "headers/line.h"


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  Global variables used by this parser.                                   */
/*                                                                          */
/*--------------------------------------------------------------------------*/

PRIVATE FILE *InputFile;           /*  CPL source comes from here.          */
PRIVATE FILE *ListFile;            /*  For nicely-formatted syntax errors.  */

PRIVATE TOKEN  CurrentToken;       /*  Parser lookahead token.  Updated by  */
                                   /*  routine Accept (below).  Must be     */
                                   /*  initialised before parser starts.    */
PRIVATE int ErrorFlag = 0;


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  Function prototypes                                                     */
/*                                                                          */
/*--------------------------------------------------------------------------*/

PRIVATE int  OpenFiles( int argc, char *argv[] );
PRIVATE void Accept( int code );
PRIVATE void Synchronise(SET *F, SET *FB);
PRIVATE void SetupSets(void);

PRIVATE SET ProgramFS_aug1;
PRIVATE SET ProgramFS_aug2;
PRIVATE SET ProgramFBS_aug;
PRIVATE SET ProcDeclarationFS_aug1;
PRIVATE SET ProcDeclarationFS_aug2;
PRIVATE SET ProcDeclarationFBS_aug;
PRIVATE SET BlockFS_aug;
PRIVATE SET BlockFBS_aug;

PRIVATE void ParseProgram(void);
PRIVATE void ParseDeclarations(void);
PRIVATE void ParseProcDeclaration(void);
PRIVATE void ParseParameterList(void);
PRIVATE void ParseFormalParameter(void);
PRIVATE void ParseBlock(void);
PRIVATE void ParseStatement(void);
PRIVATE void ParseSimpleStatement(void);
PRIVATE void ParseRestOfStatement(void);
PRIVATE void ParseProcCallList(void);
PRIVATE void ParseAssignment(void);
PRIVATE void ParseActualParameter(void);
PRIVATE void ParseWhileStatement(void);
PRIVATE void ParseIfStatement(void);
PRIVATE void ParseReadStatement(void);
PRIVATE void ParseWriteStatement(void);
PRIVATE void ParseExpression(void);
PRIVATE void ParseCompoundTerm(void);
PRIVATE void ParseTerm(void);
PRIVATE void ParseSubTerm(void);
PRIVATE void ParseBooleanExpression(void);
PRIVATE void ParseAddOp(void);
PRIVATE void ParseMultOp(void);
PRIVATE void ParseRelOp(void);


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  Main: parser entry point.  Sets up parser globals (opens input and */
/*        output files, initialises current lookahead), then calls          */
/*        "ParseProgram" to start the parse.                                */
/*                                                                          */
/*--------------------------------------------------------------------------*/

PUBLIC int main ( int argc, char *argv[] )
{
    if ( OpenFiles( argc, argv ) )  {

        InitCharProcessor( InputFile, ListFile );
        CurrentToken = GetToken();
        SetupSets();
        ParseProgram();
        fclose( InputFile );
        fclose( ListFile );
        if (!ErrorFlag) {
            printf("Valid\n");
            return EXIT_SUCCESS; /*print valid and exit success*/
        } else
            return EXIT_FAILURE; /*exit failure, no "invalid" print because
                                   error prints will be present*/
    }
    else {
        return EXIT_FAILURE;
    }
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  Parser routines: Recursive-descent implementaion of the grammar's       */
/*                   productions.                                           */
/*                                                                          */
/*                                                                          */
/*  ParseProgram implements:                                                */
/*                                                                          */
/*       <Program〉 :== “PROGRAM” 〈Identifier 〉 “;”                        */
/*                [ 〈Declarations〉 ] { 〈ProcDeclaration〉 } 〈Block 〉 “.” */
/*                                                                          */
/*--------------------------------------------------------------------------*/

PRIVATE void ParseProgram( void )
{
    Accept( PROGRAM );
    Accept( IDENTIFIER );
    Accept( SEMICOLON );

    Synchronise(&ProgramFS_aug1, &ProgramFBS_aug);
    if (CurrentToken.code == VAR)
        ParseDeclarations();
    Synchronise(&ProgramFS_aug2, &ProgramFBS_aug);
    while (CurrentToken.code == PROCEDURE) {
        ParseProcDeclaration();
        Synchronise(&ProgramFS_aug2, &ProgramFBS_aug);
    }

    ParseBlock();
    Accept( ENDOFPROGRAM );
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseDeclarations implements:                                           */
/*                                                                          */
/*  〈Declarations〉 :== “VAR” 〈Variable〉 { “,” 〈Variable〉 } “;”          */
/*                                                                          */
/*--------------------------------------------------------------------------*/

PRIVATE void ParseDeclarations( void )
{
    Accept( VAR );
    Accept( IDENTIFIER );

    while( CurrentToken.code == COMMA) {
        Accept( COMMA );
        Accept( IDENTIFIER );
    }
    Accept( SEMICOLON );
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseProcDeclaration implements:                                        */
/*                                                                          */
/*  <ProcDeclaration〉 :== “PROCEDURE” 〈Identifier                          */
/*    [ 〈ParameterList〉 ] “;” [ 〈Declarations〉 ] { 〈ProcDeclaration〉 }  */
/*    〈Block 〉 “;”                                                         */
/*                                                                          */
/*--------------------------------------------------------------------------*/

PRIVATE void ParseProcDeclaration( void )
{
    Accept( PROCEDURE );
    Accept( IDENTIFIER );

    if ( CurrentToken.code == LEFTPARENTHESIS ) {
        ParseParameterList();
    }
    Accept( SEMICOLON );
    Synchronise(&ProcDeclarationFS_aug1, &ProcDeclarationFBS_aug);
    if (CurrentToken.code == VAR) {
        ParseDeclarations();
    }
    Synchronise(&ProcDeclarationFS_aug2, &ProcDeclarationFBS_aug);
    while (CurrentToken.code == PROCEDURE ) {
        ParseProcDeclaration();
        Synchronise(&ProcDeclarationFS_aug2, &ProcDeclarationFBS_aug);
    }


    ParseBlock();
    Accept(SEMICOLON);
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseParameterList implements:                                          */
/*                                                                          */
/*  〈ParameterList〉 :== “(” 〈FormalParameter                              */
/*                        { “,” 〈FormalParameter 〉 } “)”                   */
/*                                                                          */
/*--------------------------------------------------------------------------*/

PRIVATE void ParseParameterList( void )
{
   Accept( LEFTPARENTHESIS );
   ParseFormalParameter();

   while (CurrentToken.code == COMMA) {
       Accept( COMMA );
       ParseFormalParameter();
   }
   Accept( RIGHTPARENTHESIS );
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseFormalParameter implements:                                        */
/*                                                                          */
/*  〈FormalParameter 〉 :== [ “REF” ] 〈Variable〉                           */
/*                                                                          */
/*--------------------------------------------------------------------------*/

PRIVATE void ParseFormalParameter( void )
{
    if (CurrentToken.code == REF) {
        Accept( REF );
    }
    Accept( IDENTIFIER );
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseBlock implements:                                                  */
/*                                                                          */
/*  〈Block 〉 :== “BEGIN” { 〈Statement〉 “;” } “END”                       */
/*                                                                          */
/*--------------------------------------------------------------------------*/

PRIVATE void ParseBlock( void )
{
    Accept( BEGIN );
    Synchronise(&BlockFS_aug, &BlockFBS_aug);
    while ( CurrentToken.code == IDENTIFIER || CurrentToken.code == WHILE ||
            CurrentToken.code == IF || CurrentToken.code == READ ||
            CurrentToken.code == WRITE ) {
        ParseStatement();
        Accept(SEMICOLON);
        Synchronise(&BlockFS_aug, &BlockFBS_aug);
    }

    Accept( END );
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseStatement implements:                                              */
/*                                                                          */
/*       <Statement〉 :== 〈SimpleStatement〉 | 〈WhileStatement〉 |          */
/*                    〈IfStatement〉 |〈ReadStatement〉 | 〈WriteStatement〉 */
/*                                                                          */
/*--------------------------------------------------------------------------*/

PRIVATE void ParseStatement( void )
{
    switch (CurrentToken.code) {
    case IDENTIFIER:
        ParseSimpleStatement();
        break;
    case WHILE:
        ParseWhileStatement();
        break;
    case IF:
        ParseIfStatement();
        break;
    case READ:
        ParseReadStatement();
        break;
    case WRITE:
        ParseWriteStatement();
        break;
    default:
        break;
    }
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseSimpleStatement implements:                                        */
/*                                                                          */
/*       〈SimpleStatement〉 :== 〈VarOrProcName〉 〈RestOfStatement〉        */
/*                                                                          */
/*--------------------------------------------------------------------------*/

PRIVATE void ParseSimpleStatement( void )
{
    Accept( IDENTIFIER );
    ParseRestOfStatement();
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseRestOfStatement implements:                                        */
/*                                                                          */
/*       〈RestOfStatement〉 :== 〈ProcCallList〉 | 〈Assignment〉 | \eps     */
/*                                                                          */
/*--------------------------------------------------------------------------*/

PRIVATE void ParseRestOfStatement( void )
{
    switch( CurrentToken.code ) {
        case LEFTPARENTHESIS:
            ParseProcCallList();
            break;
        case ASSIGNMENT:
            ParseAssignment();
            break;
        /***Handle epsilon here?*/
        default:
            break;
    }
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseProcCallList implements:                                           */
/*                                                                          */
/*       〈ProcCallList〉 :== “(” 〈ActualParameter                          */
/*                            { “,” 〈ActualParameter 〉 } “)”               */
/*                                                                          */
/*--------------------------------------------------------------------------*/

PRIVATE void ParseProcCallList( void )
{
    Accept( LEFTPARENTHESIS );
    ParseActualParameter();

    while (CurrentToken.code == COMMA ) {
        Accept( COMMA );
        ParseActualParameter();
    }
    Accept( RIGHTPARENTHESIS );
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseAssignment implements:                                           */
/*                                                                          */
/*       〈Assignment〉 :== “:=” 〈Expression〉                              */
/*                                                                          */
/*--------------------------------------------------------------------------*/

PRIVATE void ParseAssignment( void )
{
    Accept( ASSIGNMENT );
    ParseExpression();
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseActualParameter implements:                                        */
/*                                                                          */
/*       〈ActualParameter〉 :== 〈Variable〉 | 〈Expression〉                */
/*                                                                          */
/*--------------------------------------------------------------------------*/

PRIVATE void ParseActualParameter( void )
{
    if( CurrentToken.code == IDENTIFIER)
        Accept( IDENTIFIER );
    else
        ParseExpression();
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseWhileStatment implements:                                          */
/*                                                                          */
/*       <WhileStatement〉 :== “WHILE” 〈BooleanExpression〉 “DO” 〈Block 〉  */
/*                                                                          */
/*--------------------------------------------------------------------------*/

PRIVATE void ParseWhileStatement( void )
{
    Accept( WHILE );
    ParseBooleanExpression();
    Accept( DO );
    ParseBlock();
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseIfStatment implements:                                             */
/*                                                                          */
/*       〈IfStatement〉 :== “IF” 〈BooleanExpression〉 “THEN”               */
/*                            〈Block 〉 [ “ELSE” 〈Block 〉 ]               */
/*                                                                          */
/*--------------------------------------------------------------------------*/

PRIVATE void ParseIfStatement( void )
{
    Accept( IF );
    ParseBooleanExpression();
    Accept( THEN );
    ParseBlock();

    if (CurrentToken.code == ELSE) {
        Accept( ELSE );
        ParseBlock();
    }
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseReadStatment implements:                                           */
/*                                                                          */
/*       〈ReadStatement〉 :== “READ” “(” 〈Variable〉                       */
/*                            { “,” 〈Variable〉 } “)”                       */
/*                                                                          */
/*--------------------------------------------------------------------------*/

PRIVATE void ParseReadStatement( void )
{
    Accept( READ );
    Accept( LEFTPARENTHESIS );
    Accept( IDENTIFIER );

    while (CurrentToken.code == COMMA) {
        Accept( COMMA );
        Accept( IDENTIFIER );
    }
    Accept( RIGHTPARENTHESIS );
}

/*--------------------------------------------------------------------------*/
/*                                                                           */
/*  ParseWriteStatment implements:                                           */
/*                                                                           */
/*       〈WriteStatement〉 :== “WRITE” “(” 〈Expression〉                    */
/*                            { “,” 〈Expression〉 } “)”                     */
/*                                                                          */
/*--------------------------------------------------------------------------*/

PRIVATE void ParseWriteStatement( void )
{
    Accept( WRITE );
    Accept( LEFTPARENTHESIS );
    ParseExpression();

    while( CurrentToken.code == COMMA ) {
        Accept( COMMA );
        ParseExpression();
    }
    Accept( RIGHTPARENTHESIS );
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseExpression implements:                                             */
/*                                                                          */
/*       <Expression>  :== 〈CompoundTerm〉 { 〈AddOp〉 〈CompoundTerm〉 }    */
/*                                                                          */
/*--------------------------------------------------------------------------*/

PRIVATE void ParseExpression( void )
{
    ParseCompoundTerm();
    while( CurrentToken.code == ADD || CurrentToken.code == SUBTRACT) {
        ParseAddOp();
        ParseCompoundTerm();
    }
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseCompoundTerm implements:                                           */
/*                                                                          */
/*      〈CompoundTerm〉 :== 〈Term〉 { 〈MultOp〉 〈Term〉 }                  */
/*                                                                          */
/*--------------------------------------------------------------------------*/

PRIVATE void ParseCompoundTerm( void )
{
    ParseTerm();
    while ( CurrentToken.code == MULTIPLY || CurrentToken.code == DIVIDE) {
        ParseMultOp();
        ParseTerm();
    }
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseTerm implements:                                                   */
/*                                                                          */
/*      〈Term〉 :== [ “-” ] 〈SubTerm〉                                     */
/*                                                                          */
/*--------------------------------------------------------------------------*/

PRIVATE void ParseTerm( void )
{
    if(CurrentToken.code == SUBTRACT)
        Accept( SUBTRACT );
    ParseSubTerm();
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseSubTerm implements:                                                */
/*                                                                          */
/*      〈SubTerm〉 :== 〈Variable〉 | 〈IntConst〉 | “(” 〈Expression〉 “)”   */
/*                                                                          */
/*--------------------------------------------------------------------------*/

PRIVATE void ParseSubTerm( void )
{
    switch (CurrentToken.code)
    {
    case IDENTIFIER:
        Accept( IDENTIFIER);
        break;
    case INTCONST:
        Accept( INTCONST );
        break;
    case LEFTPARENTHESIS:
        Accept( LEFTPARENTHESIS );
        ParseExpression();
        Accept( RIGHTPARENTHESIS );
    default:
        break;
    }
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseBooleanExpression implements:                                      */
/*                                                                          */
/*      〈BooleanExpression〉 :== 〈Expression〉 〈RelOp〉 〈Expression〉      */
/*                                                                          */
/*--------------------------------------------------------------------------*/

PRIVATE void ParseBooleanExpression( void )
{
    ParseExpression();
    ParseRelOp();
    ParseExpression();
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseAddOp implements:                                                  */
/*                                                                          */
/*      〈AddOp〉 :== “+” | “-”                                              */
/*                                                                          */
/*--------------------------------------------------------------------------*/

PRIVATE void ParseAddOp( void )
{
    if ( CurrentToken.code == ADD )
        Accept( ADD );
    else if ( CurrentToken.code == SUBTRACT )
        Accept( SUBTRACT );
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseMultOp implements:                                                 */
/*                                                                          */
/*      〈MultOp〉 :== “*” | “/”                                             */
/*                                                                          */
/*--------------------------------------------------------------------------*/

PRIVATE void ParseMultOp( void )
{
    if ( CurrentToken.code == MULTIPLY)
        Accept( MULTIPLY );
    else if ( CurrentToken.code == DIVIDE)
        Accept( DIVIDE);
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseRelOp implements:                                                 */
/*                                                                          */
/*      〈RelOp〉 :== “=” | “<=” | “>=” | “<” | “>”                          */
/*                                                                          */
/*--------------------------------------------------------------------------*/

PRIVATE void ParseRelOp( void )
{
    switch (CurrentToken.code)
    {
    case EQUALITY:
        Accept( EQUALITY );
        break;
    case LESSEQUAL:
        Accept( LESSEQUAL );
        break;
    case GREATEREQUAL:
        Accept( GREATEREQUAL );
        break;
    case LESS:
        Accept( LESS );
        break;
    case GREATER:
        Accept( GREATER );
        break;
    default:
        break;
    }
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  End of parser.  Support routines follow.                                */
/*                                                                          */
/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  Accept:  Takes an expected token name as argument, and if the current   */
/*           lookahead matches this, advances the lookahead and returns.    */
/*                                                                          */
/*           If the expected token fails to match the current lookahead,    */
/*           this routine reports a syntax error and exits ("crash & burn"  */
/*           parsing).  Note the use of routine "SyntaxError"               */
/*           (from "scanner.h") which puts the error message on the         */
/*           standard output and on the listing file, and the helper        */
/*           "ReadToEndOfFile" which just ensures that the listing file is  */
/*           completely generated.                                          */
/*                                                                          */
/*                                                                          */
/*    Inputs:       Integer code of expected token                          */
/*                                                                          */
/*    Outputs:      None                                                    */
/*                                                                          */
/*    Returns:      Nothing                                                 */
/*                                                                          */
/*    Side Effects: If successful, advances the current lookahead token     */
/*                  "CurrentToken".                                         */
/*                                                                          */
/*--------------------------------------------------------------------------*/

PRIVATE void Accept( int ExpectedToken )
{
    static int recovering = 0;
    if (recovering) {
        while(CurrentToken.code != ExpectedToken && CurrentToken.code !=
              ENDOFINPUT)
            CurrentToken = GetToken();

        recovering = 0;
    }
    if ( CurrentToken.code != ExpectedToken )  {
        SyntaxError( ExpectedToken, CurrentToken );
        recovering = 1;
        ErrorFlag = 1;
    }
    else  CurrentToken = GetToken();
}

PRIVATE void SetupSets(void)
{

    /* Set for synching ParseProgram */
    InitSet(&ProgramFS_aug1, 3, VAR, PROCEDURE, BEGIN);
    InitSet(&ProgramFS_aug2, 2, PROCEDURE, BEGIN);
    InitSet(&ProgramFBS_aug, 3, ENDOFPROGRAM, ENDOFINPUT, END);

    /* Set for synching ParseProcDeclaration */
    InitSet(&ProcDeclarationFS_aug1, 3, VAR, PROCEDURE, BEGIN);
    InitSet(&ProcDeclarationFS_aug2, 2, PROCEDURE, BEGIN);
    InitSet(&ProcDeclarationFBS_aug, 3, ENDOFPROGRAM, ENDOFINPUT, END);

    /* Set for synching ParseBlock */
    InitSet(&BlockFS_aug, 6, IDENTIFIER, WHILE, IF, READ, WRITE, END);
    InitSet(&BlockFBS_aug, 4, ENDOFINPUT, ENDOFPROGRAM, SEMICOLON, ELSE);
}

PRIVATE void Synchronise(SET *F, SET *FB)
{
    SET S;

    S = Union(2, F, FB);
    if(!InSet(F, CurrentToken.code)) {
        SyntaxError2(*F, CurrentToken);
        while(!InSet(&S, CurrentToken.code))
            CurrentToken = GetToken();
    }
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  OpenFiles:  Reads strings from the command-line and opens the           */
/*              associated input and listing files.                         */
/*                                                                          */
/*    Note that this routine mmodifies the globals "InputFile" and          */
/*    "ListingFile".  It returns 1 ("true" in C-speak) if the input and     */
/*    listing files are successfully opened, 0 if not, allowing the caller  */
/*    to make a graceful exit if the opening process failed.                */
/*                                                                          */
/*                                                                          */
/*    Inputs:       1) Integer argument count (standard C "argc").          */
/*                  2) Array of pointers to C-strings containing arguments  */
/*                  (standard C "argv").                                    */
/*                                                                          */
/*    Outputs:      No direct outputs, but note side effects.               */
/*                                                                          */
/*    Returns:      Boolean success flag (i.e., an "int":  1 or 0)          */
/*                                                                          */
/*    Side Effects: If successful, modifies globals "InputFile" and         */
/*                  "ListingFile".                                          */
/*                                                                          */
/*--------------------------------------------------------------------------*/

PRIVATE int  OpenFiles( int argc, char *argv[] )
{

    if ( argc != 3 )  {
        fprintf( stderr, "%s <inputfile> <listfile>\n", argv[0] );
        return 0;
    }

    if ( NULL == ( InputFile = fopen( argv[1], "r" ) ) )  {
        fprintf( stderr, "cannot open \"%s\" for input\n", argv[1] );
        return 0;
    }

    if ( NULL == ( ListFile = fopen( argv[2], "w" ) ) )  {
        fprintf( stderr, "cannot open \"%s\" for output\n", argv[2] );
        fclose( InputFile );
        return 0;
    }

    return 1;
}
