/*--------------------------------------------------------------------------*/
/*                                                                          */
/*       parser1                                                            */
/*                                                                          */
/*                                                                          */
/*       Group Members:          ID numbers                                 */
/*                                                                          */
/*       Ronan Randles            19242441                                  */
/*                                                                          */
/*--------------------------------------------------------------------------*/
/*                                                                          */
/*       parser1 Grammar:                                                   */
/*                                                                          */
/*  〈Program〉 :== “PROGRAM” 〈Identifier 〉 “;”
                    [ 〈Declarations〉 ] { 〈ProcDeclaration〉 } 〈Block 〉 “.”

    〈Declarations〉 :== “VAR” 〈Variable〉 { “,” 〈Variable〉 } “;”
    〈ProcDeclaration〉 :== “PROCEDURE” 〈Identifier 〉 [ 〈ParameterList〉 ] “;”
                        [ 〈Declarations〉 ] { 〈ProcDeclaration〉 } 〈Block 〉 “;”

    〈ParameterList〉 :== “(” 〈FormalParameter 〉 { “,” 〈FormalParameter 〉 } “)”
    〈FormalParameter 〉 :== [ “REF” ] 〈Variable〉
    〈Block 〉 :== “BEGIN” { 〈Statement〉 “;” } “END”
    〈Statement〉 :== 〈SimpleStatement〉 | 〈WhileStatement〉 | 〈IfStatement〉 |
                    〈ReadStatement〉 | 〈WriteStatement〉

    〈SimpleStatement〉 :== 〈VarOrProcName〉 〈RestOfStatement〉
    〈RestOfStatement〉 :== 〈ProcCallList〉 | 〈Assignment〉 | \eps
    〈ProcCallList〉 :== “(” 〈ActualParameter 〉 { “,” 〈ActualParameter 〉 } “)”
    〈Assignment〉 :== “:=” 〈Expression〉
    〈ActualParameter 〉 :== 〈Variable〉 | 〈Expression〉
    〈WhileStatement〉 :== “WHILE” 〈BooleanExpression〉 “DO” 〈Block 〉
    〈IfStatement〉 :== “IF” 〈BooleanExpression〉 “THEN” 〈Block 〉 [ “ELSE” 〈Block 〉 ]
    〈ReadStatement〉 :== “READ” “(” 〈Variable〉 { “,” 〈Variable〉 } “)”
    〈WriteStatement〉 :== “WRITE” “(” 〈Expression〉 { “,” 〈Expression〉 } “)”
    〈Expression〉 :== 〈CompoundTerm〉 { 〈AddOp〉 〈CompoundTerm〉 }
    〈CompoundTerm〉 :== 〈Term〉 { 〈MultOp〉 〈Term〉 }
    〈Term〉 :== [ “-” ] 〈SubTerm〉
    〈SubTerm〉 :== 〈Variable〉 | 〈IntConst〉 | “(” 〈Expression〉 “)”
    〈BooleanExpression〉 :== 〈Expression〉 〈RelOp〉 〈Expression〉
    〈AddOp〉 :== “+” | “-”
    〈MultOp〉 :== “*” | “/”
    〈RelOp〉 :== “=” | “<=” | “>=” | “<” | “>”
    〈Variable〉 :== 〈Identifier 〉
    〈VarOrProcName〉 :== 〈Identifier 〉
    〈Identifier 〉 :== 〈Alpha〉 { 〈AlphaNum〉 }
    〈IntConst〉 :== 〈Digit〉 { 〈Digit〉 }
    〈AlphaNum〉 :== 〈Alpha〉 | 〈Digit〉
    〈Alpha〉 :== “A” . . . “Z” | “a” . . . “z”
    〈Digit〉 :== “0” . . . “9”                                              */
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


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  Function prototypes                                                     */
/*                                                                          */
/*--------------------------------------------------------------------------*/

PRIVATE int  OpenFiles( int argc, char *argv[] );
PRIVATE void Accept( int code );
PRIVATE void ReadToEndOfFile( void );

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
PRIVATE void ParseVariable(void);
PRIVATE void ParseIntConst(void);
PRIVATE void ParseIdentifier(void);


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
        ParseProgram();
        fclose( InputFile );
        fclose( ListFile );
        return  EXIT_SUCCESS;
    }
    else
        return EXIT_FAILURE;
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
/*                                                                          */                                                                        */
/*--------------------------------------------------------------------------*/

PRIVATE void ParseProgram( void )
{
    //TODO
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
    //TODO
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

PRIVATE void ParseProdeclarations( void )
{
    //TODO
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
    //TODO
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
    //TODO
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
    //TODO
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
    //TODO
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
    //TODO
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
    //TODO
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
    //TODO
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
    //TODO
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
    //TODO
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
    //TODO
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
    //TODO
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
    //TODO
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
    //TODO
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
    //TODO
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
    //TODO
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
    //TODO
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseSubTerm implements:                                                */
/*                                                                          */
/*      〈SubTerm〉 :== 〈Variable〉 | 〈IntConst〉 | “(” 〈Expression〉 “)”   */                                     */
/*                                                                          */
/*--------------------------------------------------------------------------*/

PRIVATE void ParseSubTerm( void )
{
    //TODO
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseBooleanExpression implements:                                      */
/*                                                                          */
/*      〈BooleanExpression〉 :== 〈Expression〉 〈RelOp〉 〈Expression〉      */                                     */
/*                                                                          */
/*--------------------------------------------------------------------------*/

PRIVATE void ParseBooleanExpression( void )
{
    //TODO
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
    //TODO
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
    //TODO
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
    //TODO
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseVariable implements:                                               */
/*                                                                          */
/*      〈Variable〉 :== 〈Identifier 〉                                     */
/*                                                                          */
/*--------------------------------------------------------------------------*/

PRIVATE void ParseVariable( void )
{
    //TODO
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseVarOrProcName implements:                                          */
/*                                                                          */
/*      〈VarOrProcName〉 :== 〈Identifier 〉                                */
/*                                                                          */
/*--------------------------------------------------------------------------*/

PRIVATE void ParseVarOrProcName( void )
{
    //TODO
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseIdentifier implements:                                             */
/*                                                                          */
/*      〈Identifier 〉 :== 〈Alpha〉 { 〈AlphaNum〉 }                        */
/*                                                                          */
/*--------------------------------------------------------------------------*/

PRIVATE void ParseIdentifier( void )
{
    //TODO
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseIntConst implements:                                               */
/*                                                                          */
/*      〈IntConst〉 :== 〈Digit〉 { 〈Digit〉 }                              */
/*                                                                          */
/*--------------------------------------------------------------------------*/

PRIVATE void ParseIntConst( void )
{
    //TODO
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseAlpha implements:                                                  */
/*                                                                          */
/*      〈AlphaNum〉 :== 〈Alpha〉 | 〈Digit〉                                */
/*                                                                          */
/*--------------------------------------------------------------------------*/

PRIVATE void ParseAlphaNum( void )
{
    //TODO
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
    if ( CurrentToken.code != ExpectedToken )  {
        SyntaxError( ExpectedToken, CurrentToken );
        ReadToEndOfFile();
        fclose( InputFile );
        fclose( ListFile );
        exit( EXIT_FAILURE );
    }
    else  CurrentToken = GetToken();
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


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ReadToEndOfFile:  Reads all remaining tokens from the input file.       */
/*              associated input and listing files.                         */
/*                                                                          */
/*    This is used to ensure that the listing file refects the entire       */
/*    input, even after a syntax error (because of crash & burn parsing,    */
/*    if a routine like this is not used, the listing file will not be      */
/*    complete.  Note that this routine also reports in the listing file    */
/*    exactly where the parsing stopped.  Note that this routine is         */
/*    superfluous in a parser that performs error-recovery.                 */
/*                                                                          */
/*                                                                          */
/*    Inputs:       None                                                    */
/*                                                                          */
/*    Outputs:      None                                                    */
/*                                                                          */
/*    Returns:      Nothing                                                 */
/*                                                                          */
/*    Side Effects: Reads all remaining tokens from the input.  There won't */
/*                  be any more available input after this routine returns. */
/*                                                                          */
/*--------------------------------------------------------------------------*/

PRIVATE void ReadToEndOfFile( void )
{
    if ( CurrentToken.code != ENDOFINPUT )  {
        Error( "Parsing ends here in this program\n", CurrentToken.pos );
        while ( CurrentToken.code != ENDOFINPUT )  CurrentToken = GetToken();
    }
}
