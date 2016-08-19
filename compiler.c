//this is a tiny compiler
//although it can work, but it is not perfect
//it does not support all the syntax
// such as for(..){..} and int a = 8(int a;a=8;);
//i do this for interesting.
//2016.8.17

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

int token;	//current token
int token_val;	//token value
int line;	//line number of the source code file
char *src;	//pointer of code
int *text,	//text segement
	*stack;	//stack

char *data;	//data segement
int basetype;	//base type the id
int expr_type;	//the type of expression

int *sp, *bp, *pc, ax;	//register

int poolsize;
int index_of_bp;

int *symbols,		//symbol table
	*current_id;	//current token id;
int *idmain;	//keep track of main

//instructions
enum {	LEA, IMM, JMP, CALL, JZ, JNZ, ENT, ADJ, LEV, LI, LC, SI, SC, PUSH,
		OR, XOR, AND, EQ, NE, LT, GT, LE, GE, SHL, SHR, ADD, SUB, MUL, DIV, MOD,
		OPEN, READ, CLOS, PRTF, MALC, MSET, MCMP, EXIT };

//token
enum {	Num = 128, Fun, Sys, Glo, Loc, Id,
		Char, Else, Enum, If, Int, Return, Sizeof, While,
		Assign, Cond, Lor, Lan, Or, Xor, And, Eq, Ne, Lt, Gt, Le, Ge,
		Shl, Shr, Add, Sub, Mul, Div, Mod, Inc, Dec, Brak };
//token id
enum {	Token, Hash, Name, Type, Class, Value, BType, BClass, BValue, IdSize };

//type
enum {	CHAR, INT, PTR };

void next(){
	//1.#
	//2.//
	//3.num
	//4.id
	//5.".." or '..'
	//6.operator
	//

	char *last_pos;
	int hash;

	while(token = *src){
		
		src++;

		if(token == '\n'){
			line++;
		}
		else if(token == '#'){
			//#include or #define ... just skip
			while(*src != 0 && *src != '\n'){
				src++;
			}
		}
		else if((token >='a' && token <= 'z') || (token >= 'A' && token <= 'Z') ||(token == '_')){
			//id: int.. variable enum func..
			hash = token;
			last_pos = src - 1;
			while((*src >= 'a' && *src <= 'z') || (*src >= 'A' && *src <= 'Z') || (*src >= '0' && *src <= '9') || (*src == '_')){
				hash = hash * 147 + *src++;
			}

			current_id = symbols;
			while(current_id[Token]){
				//find if the id exist
				if(current_id[Hash] == hash && memcmp((char *)current_id[Name], last_pos, src - last_pos) == 0){
					//id exist.
					token = current_id[Token];
					return;
				}

				current_id = current_id + IdSize;
			}

			//store new id
			token = current_id[Token] = Id;
			current_id[Hash] = hash;
			current_id[Name] = (int)last_pos;
			return;
		}//end if id
		else if(token >= '0' && token <= '9'){
			// number:hex, oct, dec
			token_val = token - '0';
			if(token_val > 0){
				//dec
				while(*src >= '0' && *src <= '9'){
					token_val = token_val * 10 + *src - '0';
					src++;
				}
			}else if(*src == 'x' || *src == 'X'){
				//hex
				token = *++src;
				while((token >= '0'&& token <= '9') || (token >= 'A' && token <= 'F') || (token >= 'a' && token <= 'f')){
					token_val = token_val * 16 + (token & 15) + (token >= 'A'? 9 : 0);
					token = *++src;
				}
			}else{
				//oct
				while(*src >= '0' && *src <= '7'){
					token_val = token_val * 8 + (*src - '0');
					src++;
				}
			}

			token = Num;
			return;
		}// end if num
		else if(token == '"' || token == '\''){
			//"...\n.." 'a' '\n'
			last_pos = data;
			while(*src != 0 && *src != token){
				
				token_val = *src++;	
				if(token_val == '\\'){
					
					token_val = *src++;
					if(token_val == 'n'){
						token_val = '\n';
					}
				}

				if(token == '"'){
					*data++ = token_val;
				}
			}

			src++;	//skip " or '
			// like 'a' is number, like "..." token_val is the address of the data stored the string
			if(token == '"'){
				token_val = (int)last_pos;
			}else{
				token = Num;
			}
			return;
		}//end " '
		else if(token == '/'){
			// may be '//' and '/'
			if(*src == '/'){
				//comment
				while(*src != 0 && *src != '\n'){
					src++;
				}
			} else {
				//divide
				token = Div;
				return;
			}
		}
		else if(token == '='){
			//maybe '=' or '=='
			if(*src == '='){
				// '=='
				src++;
				token = Eq;
			}else{
				// '='
				token = Assign;
			}
			return;
		}
		else if(token == '|'){
			//maybe '|' or '||'
			if(*src == '|'){
				//'||'
				src++;
				token = Lor;
			} else{
				//'|'
				token = Or;
			}
			return;
		}
		else if(token == '&'){
			//maybe '&' or '&&'
			if(*src == '&'){
				//'&&'
				src++;
				token = Lan;
			}else{
				// '&'
				token = And;
			}
			return;
		}
		else if(token == '!'){
			// maybe '!' or '!='
			if(*src == '='){
				//'!='
				src++;
				token = Ne;
			} else {
				token = '!';
			}
			return;
		}
		else if(token == '<'){
			//maybe '<' or '<<' or '<='
			if(*src == '<'){
				// '<<'
				src++;
				token = Shl;
			} else if(*src == '='){
				// '<='
				src++;
				token = Le;
			} else {
				token = Lt;
			}
			return;
		}
		else if(token == '>'){
			//maybe '>' or '>>' or '>='
			if(*src == '>'){
				// '>>'
				src++;
				token = Shr;
			} else if(*src == '='){
				// '>='
				src++;
				token = Ge;
			} else {
				token = Gt;
			}
			return;
		}
		else if(token == '+'){
			//maybe '+' or '++'
			if(*src == '+'){
				// '++'
				src++;
				token = Inc;
			} else {
				token = Add;
			}
			return;
		}
		else if(token == '-'){
			//maybe '-' or '--'
			if(*src == '-'){
				// '--'
				src++;
				token = Dec;
			} else {
				token = Sub;
			}
			return;
		}
		else if(token == '*'){
			token = Mul;
			return;
		}
		else if(token == '%'){
			token = Mod;
			return;
		}
		else if(token == '['){
			token = Brak;
			return;
		}
		else if(token == '?'){
			token = Cond;
			return;
		}
		else if(token == '^'){
			token = Xor;
			return;
		}
		else if(token == '{' || token == '}' || token == ',' || token == ';' || token == ']' || token == '~' || token == '(' || token == ')' || token == ':'){
			return;
		}
	}//end while
	return;
}
void match(int tk){
	if(token == tk){
		next();
	}else{
		printf("token:%c, tk:%c, not match token at line:%d\n", token, tk, line);
		exit(-1);
	}
}

//1.unit(variable or function call) or unary(+var -var !var)
//2.binary
void expression(int level){
	
	int *id;
	int *addr;
	int arg_num;
	int tmp_type;
	int tmp_token;
	//unit or unary
	{
		if(!token){
			printf("unknow token at line:%d\n",line);
			exit(-1);
		}
		//1.num
		//2."string"
		//3.sizeof()
		//4.id
		//	global/local variable
		//	function call
		//	enum number
		//5.+expression
		//6.-expression
		//7.++expression
		//8.--expression
		//9.~expression
		//10.!expression
		//11.(int *) or (expression)
		//12.&var
		//13.*var
		if(token == Num){
			//immediate number
			*++text = IMM;
			*++text = token_val;
			match(Num);
			expr_type = INT;
		}
		else if(token == '"'){
			// "abc" "cde"
			
			*++text = IMM;
			*++text = token_val;

			match('"');

			//maybe "abc"+"cde"
			if(token == '"'){
				match('"');
			}

			//set the end of the string
			//move 4 bit forward, for data segement set to 0, and need to 4 bit 对齐
			data = (char *)(((int)data + sizeof(int)) & (-sizeof(int)));
			expr_type = PTR;
		}
		else if(token == Sizeof){
			//sizeof() only support int or char or int ** char **

			match(Sizeof);
			match('(');

			if(token == Int){
				expr_type = INT;
				match(Int);
			}else if(token == Char){
				expr_type = CHAR;
				match(Char);
			}

			//maybe int **
			while(token == Mul){
				match(Mul);
				expr_type = expr_type + PTR;
			}

			*++text = IMM;
			*++text = (expr_type == CHAR) ? sizeof(char) : sizeof(int);

			expr_type = INT;

		}
		else if(token == Id){
			//1. function call(fun or sys)
			//2. enum number
			//3. global/local variable

			id = current_id;
			match(Id);

			if(token == '('){
				//function call (fun or sys)

				//the number of arguments
				arg_num = 0;
				match('(');
				while(token != ')'){
					expression(Assign);

					*++text = PUSH;
					if(token == ','){
						match(',');
					}

					arg_num++;

				}

				match(')');

				if(id[Class] == Fun){
					//function call
					*++text = CALL;
					*++text = id[Value];
				} else if(id[Class] == Sys){
					//system function
					*++text = id[Value];
				} else {
					printf("invalid function call at line:%d\n",line);
					exit(-1);
				}

				if(arg_num > 0){
					*++text = ADJ;
					*++text = arg_num;
				}

				expr_type = id[Type];

			//end function call
			} else if(id[Class] == Num){
				//enum number
				*++text = IMM;
				*++text = id[Value];
				expr_type = Num;

			} else {
				//global/local variable
				if(id[Class] == Loc){
					//local variable
					*++text = LEA;
					*++text = index_of_bp - id[Value];	//saving in the stack
				} else if(id[Class] == Glo){
					//global variable
					*++text = IMM;
					*++text = id[Value];	//saving in the data segement
				} else {
					printf("invalid variable using at line:%d\n",line);
					exit(-1);
				}

				*++text = (id[Type] == CHAR) ? LC : LI;
				expr_type = id[Type];
			}
		}//end token == Id
		else if(token == '('){
			//1.like (char *)
			//2.(expression)
			match('(');

			if(token == Int || token == Char){
				
				tmp_type = (token == Int) ? INT : CHAR;
				match(token);

				//maybe (int *)
				while(token == Mul){
					match(Mul);
					tmp_type = tmp_type + PTR;
				}

				match(')');

				expression(Inc);

				expr_type = tmp_type;

			} else {
				//like (expression)
				expression(Assign);
				match(')');
			}
		//end token == '('
		}
		else if(token == Mul){
			// **var

			match(Mul);
			expression(Inc);

			if(expr_type > PTR){
				expr_type = expr_type - PTR;
			}

			*++text = (expr_type == CHAR) ? LC : LI;

		}
		else if(token == And){
			// &var

			match(And);

			expression(Inc);

			if(*text == LC || *text == LI){
				--text;
			} else {
				printf("invalid (&var), can not get address of at in line:%d\n",line);
				exit(-1);
			}

			expr_type = expr_type + PTR;
		}
		else if(token == Add){
			// +var, do nothing or ()
			match(Add);
			expression(Inc);

			expr_type = INT;

		}
		else if(token == Sub){
			//-var or -expression
			match(Sub);

			*++text = IMM;
			*++text = -1;
			*++text = PUSH;
			expression(Inc);
			*++text = MUL;

			expr_type = INT;
		}
		else if(token == '~'){
			// ~var
			match('~');

			*++text = IMM;
			*++text = -1;
			*++text = PUSH;
			expression(Inc);
			*++text = XOR;

			expr_type = INT;
		}
		else if(token == '!'){
			// !var
			match('!');

			expression(Inc);

			*++text = PUSH;
			*++text = IMM;
			*++text = 0;
			*++text = LE;

			expr_type = INT;

		}
		else if(token == Inc || token == Dec){
			//++var or --var
			
			tmp_token = token;
			match(token);
			expression(Inc);
				
			if(*text == LC){
				*text = PUSH;
				*++text = LC;
			} else if(*text = LI){
				*text = PUSH;
				*++text = LI;
			} else {
				printf("invalid (++var or --var) at line:%d\n",line);
				exit(-1);
			}

			*++text = PUSH;
			*++text = IMM;
			*++text = (expr_type > PTR) ? sizeof(int) : sizeof(char);
			*++text = (tmp_token == Inc) ? ADD : SUB;
			*++text = (expr_type == CHAR) ? SC : SI;
		}
		else {
			printf("invalid expression at line:%d\n",line);
			exit(-1);
		}
	}//end unit_unary

	//binary operator
	//1.expr = expr
	//2.expr ? expr : expr
	//3.expr || expr
	//4.epxr && expr
	//5.expr | expr
	//6.expr & expr
	//7.expr == expr
	//8.expr != expr
	//9.expr < expr
	//10.expr >expr
	//11.expr <= expr
	//12.expr >= expr
	//13.expr << expr
	//14.expr >> expr
	//15.expr + expr
	//16.expr - expr
	//17.expr * expr
	//18.expr / expr
	//19.expr % expr
	//20.expr++
	//21.expr--
	//22.expr[..]
	{
		while(token >= level){
			
			tmp_type = expr_type;

			if(token == Assign){
				//expr = expr
				if(*text == LC || *text == LI){
					*text = PUSH;
				} else {
					printf("invalid assigment at line:%d\n", line);
					exit(-1);
				}

				match(Assign);

				expression(Cond);

				*++text = (tmp_type == CHAR) ? SC : SI;
				expr_type = tmp_type;
			}
			else if(token == Cond){
				//expr ? epxr : expr

				match(Cond);
				*++text = JZ;
				addr = ++text;
				expression(Assign);
				
				if(token == ':'){
					match(':');
				} else {
					printf("invalid condition jump at line:%d\n", line);
					exit(-1);
				}
				*addr = (int)(text + 3);
				*++text = JMP;
				addr = ++text;
				expression(Assign);
				*addr = (int)(text + 1);
			}
			else if(token == Lor){
				// expr || expr

				match(Lor);

				*++text = JNZ;
				addr = ++text;
				expression(Lan);
				*addr = (int)(text + 1);
				expr_type = INT;
			}
			else if(token == Lan){
				// expr && expr
				
				match(Lan);

				*++text = JZ;
				addr = ++text;
				expression(Or);
				*addr = (int)(text + 1);
				expr_type = INT;
			}
			else if(token == Or){
				//expr | expr

				match(Or);
				*++text = PUSH;
				expression(Xor);
				*++text = OR;

				expr_type = INT;
			}
			else if(token == Xor){
				// expr ^ expr
				match(Xor);
				*++text = PUSH;
				expression(And);
				*++text = XOR;

				expr_type = INT;
			}
			else if(token == And){
				//expr & expr
				match(And);
				*++text = PUSH;
				expression(Eq);
				*++text = AND;

				expr_type = INT;
			}
			else if(token == Eq){
				//expr == expr
				match(Eq);
				*++text = PUSH;
				expression(Ne);
				*++text = EQ;

				expr_type = INT;
			}
			else if(token == Ne){
				//expr != expr
				match(Ne);
				*++text = PUSH;
				expression(Lt);
				*++text = NE;

				expr_type = INT;
			}
			else if(token == Lt){
				//expr < expr
				match(Lt);
				*++text = PUSH;
				expression(Shl);
				*++text = LT;

				expr_type = INT;
			}
			else if(token == Gt){
				//expr > expr
				match(Gt);
				*++text = PUSH;
				expression(Shl);
				*++text = GT;

				expr_type = INT;
			}
			else if(token == Le){
				//expr <= expr
				match(Le);
				*++text = PUSH;
				expression(Shl);
				*++text = LE;

				expr_type = INT;
			}
			else if(token == Ge){
				//expr >= expr
				match(Ge);
				*++text = PUSH;
				expression(Shl);
				*++text = GE;

				expr_type = INT;
			}
			else if(token == Shl){
				//expr << expr
				match(Shl);
				*++text = PUSH;
				expression(Add);
				*++text = SHL;

				expr_type = INT;
			}
			else if(token == Shr){
				//expr >> expr
				match(Shr);
				*++text = PUSH;
				expression(Add);
				*++text = SHR;

				expr_type = INT;
			}
			else if(token == Add){
				//expr + expr
				match(Add);
				*++text = PUSH;
				tmp_type = expr_type;
				expression(Mul);
				
				//if(tmp_type > PTR){
					//pointer plus number
					//*++text = PUSH;

					//if((tmp_type - INT) % PTR == 0){
						// type int *
						//*++text = IMM;
						//*++text = sizeof(int);
						//*++text = MUL;
					//} else {
						//type char *
						//*++text = IMM;
						//*++text = sizeof(char);
						//*++text = MUL;
					//}

				//}

				if(tmp_type > PTR){
					//pointer type, and not char*
					*++text = PUSH;

					*++text = IMM;
					*++text = sizeof(int);
					*++text = MUL;
				}

				*++text = ADD;

				expr_type = tmp_type;
			}
			else if(token == Sub){
				//expr - expr
				match(Sub);
				tmp_type = expr_type;
				*++text = PUSH;
				expression(Mul);

				if(tmp_type > PTR && tmp_type == expr_type){
					//ptr1 - ptr2
					
					*++text = SUB;

					*++text = PUSH;
					*++text = IMM;
					*++text = sizeof(int);
					*++text = DIV;

					expr_type = INT;

					//if((tmp_type - INT) % PTR == 0){
						//type int *
						//*++text = IMM;
						//*++text = sizeof(int);
						//*++text = MUL;
					//} else {
						//type char *
						//*++text = IMM;
						//*++text = sizeof(char);
						//*++text = MUL;
					//}
					
				} else if(tmp_type > PTR){
					//ptr - num
					*++text = PUSH;
					*++text = IMM;
					*++text = sizeof(int);
					*++text = MUL;
					*++text = SUB;

					expr_type = tmp_type;
				} else {
					//num - num
					*++text = SUB;

					expr_type = INT;
				}

			}
			else if(token == Mul){
				// epxr * expr
				match(Mul);
				*++text = PUSH;
				expression(Inc);
				*++text = MUL;

				expr_type = INT;
			}
			else if(token == Div){
				//expr / expr
				match(Div);
				*++text = PUSH;
				expression(Inc);
				*++text = DIV;

				expr_type = INT;
			}
			else if(token == Mod){
				// expr % expr
				match(Mod);
				*++text = PUSH;
				expression(Inc);
				*++text = MOD;

				expr_type = INT;
			}
			else if(token == Inc || token == Dec){
				// expr++ or expr--
				tmp_token = token;

				if(*text == LC){
					*text = PUSH;
					*++text = LC;
				} else if(*text == LI){
					*text = PUSH;
					*++text = LI;
				} else {
					printf("invalid postfix increace at line:%d\n", line);
					exit(-1);
				}

				*++text = PUSH;
				*++text = IMM;
				*++text = (expr_type > PTR) ? sizeof(int) : sizeof(char);
				*++text = (tmp_token == Inc) ? ADD : SUB;
				*++text = (expr_type == CHAR) ? SC : SI;
				*++text = PUSH;
				*++text = IMM;
				*++text = (expr_type > PTR) ? sizeof(int) : sizeof(char);
				*++text = (tmp_token == Inc) ? SUB : ADD;

				match(token);
			}
			else if(token == Brak){
				//var[xx]
				match(Brak);
				*++text = PUSH;
				expression(Assign);
				match(']');
				if(tmp_type > PTR){
				
					*++text = PUSH;
					*++text = IMM;
					*++text = sizeof(int);
					*++text = MUL;
				} else {
					printf("invalid var[] at line:%d\n", line);
					exit(-1);
				}

				*++text = ADD;
				expr_type = tmp_type - PTR;
				*++text = (expr_type == CHAR) ? LC : LI;

			}
			else{
				printf("invalid token type(binary operator) at line:%d\n", line);
				exit(-1);
			}
		}//end while(token >=levle)
	}//end binary operator
}

//1.if(..){statement}[else{statement}]
//2.while(..){statement}
//3.return ...
//4.var = xxx or function call
//5.{statement}
//6.empty statement
void statement(){
	
	int *addr_a, *addr_b;

	if(token == If){
		// JZ addr_b
		// <true statement>
		//JMP a
		//else
		//addr_b:
		// false statement
		//a:

		match(If);
		match('(');
		//parse condition
		expression(Assign);
		match(')');

		*++text = JZ;
		addr_b = ++text;	//address of false statement
		
		statement();	//true statement
		
		//else <statement>
		if(token == Else){
			match(Else);
			
			*addr_b = (int)(text + 3);	//set address of false statement
			*++text = JMP;
			addr_b = ++text;

			statement();	//false statement

		}

		*addr_b = (int)(text + 1);
	}//end token == If
	else if(token == While){
		//a:
		//JZ b
		//<statement>
		//JMP a
		//b:

		match(While);
		match('(');
		addr_a = text + 1;
		//parse condition
		expression(Assign);
		match(')');

		*++text = JZ;		//jump if the ax is zero, that condition is false
		addr_b = ++text;	//store the address after while(){}

		statement();		//while(){statement}

		*++text = JMP;		//jump a 
		*++text = (int)addr_a;

		*addr_b = (int)(text + 1);

	}//end token == While
	else if(token == Return){
		//return <expression>;
		match(Return);
		if(token != ';'){
			expression(Assign);
		}
		match(';');

		//emit code, leaving the function
		*++text = LEV;
	}
	else if(token == '{'){
		//{<statement>}

		match('{');
		while(token != '}'){
			statement();
		}
		match('}');
	}
	else if(token == ';'){
		//empty statement
		match(';');
	}
	else{
		//var = a; or function call
		expression(Assign);
		match(';');
	}

}

//1.variable declaration
//2.statement
void function_body(){
	
	int local_pos;
	int type;
	//type = INT;
	local_pos = index_of_bp;

	while(token == Int || token == Char){
		type = INT;
		//local_pos = index_of_bp;
		
		if(token == Int){
			match(Int);
		}else if(token == Char){
			match(Char);
			type = CHAR;
		}

		//like int a or char a or int **a	
		while(token != ';'){

			//like int **ptr
			while(token == Mul){
				match(Mul);
				type = type + PTR; 
			}

			if(token != Id){
				printf("invalid variable declaration(invalid id) at line:%d\n",line);
				exit(-1);
			}

			if(current_id[Class] == Loc){
				printf("invalid variable declaration(id exist) at line:%d\n",line);
				exit(-1);
			}

			match(Id);

			//save global variable and reset the local variable
			current_id[BValue] = current_id[Value];
			current_id[BType] = current_id[Type];
			current_id[BClass] = current_id[Class];
			current_id[Value] = ++local_pos;
			current_id[Class] = Loc;
			current_id[Type] = type;

			if(token == ','){
				match(',');
			}
		}

		match(';');
	}//end while token == Int

	//save the local variable size in the stack
	*++text = ENT;
	*++text = local_pos - index_of_bp;

	//parse statement
	while(token != '}'){
		statement();
	}

	//leave the function
	*++text = LEV;
}

//type {*} id {, type {*}id}
void function_params(){
	int type;
	int params;
	type = INT;
	params = 0;

	while(token != ')'){
		
		if(token == Int){
			match(Int);
		}else if(token == Char){
			match(Char);
			type = CHAR;
		}

		//like int **ptr;
		while(token == Mul){
			match(Mul);
			type = type + PTR;
		}

		if(token != Id){
			printf("invaliable parameter declaration at line:%d\n",line);
			exit(-1);
		}

		//local exist;
		if(current_id[Class] == Loc){
			printf("invaliable parameter declaration at line:%d\n",line);
			exit(-1);
		}

		match(Id);

		//store the global variable and set local parameter
		current_id[BType] = current_id[Type];
		current_id[BValue] = current_id[Value];
		current_id[BClass] = current_id[Class];
		current_id[Class] = Loc;
		current_id[Type] = type;
		current_id[Value] = params++;

		if(token == ','){
			match(',');
		}
	}

	index_of_bp = params + 1;
}


//type {*}id (parameter_dec){body_dec}
void function_declaration(){

	match('(');
	function_params();
	match(')');
	match('{');
	function_body();
	//match('}'), for the global_declaration() 

	//set local variable to glo or unset it
	current_id = symbols;
	while(current_id[Token]){
		if(current_id[Class] == Loc){
			current_id[Class] = current_id[BClass];
			current_id[Type] = current_id[BType];
			current_id[Value] = current_id[BValue];
		}
		current_id = current_id + IdSize;
	}
}

void enum_declaration(){
	//enum [id] { id [= num] {, id[= num]}}
	//			|	...					|

	int enum_val;
	enum_val = 0;
	while(token != '}'){
		
		if(token != Id){
			printf("invaliable enum declaration at line:%d\n",line);
			exit(-1);
		}

		match(Id);

		if(token == Assign){
			// like a=1;
			match(Assign);
			if(token != Num){
				printf("invaliable enum num declaration at line:%d\n",line);
				exit(-1);
			}

			enum_val = token_val;
			match(Num);
		}

		current_id[Value] = enum_val++;
		current_id[Class] = Num;
		current_id[Type] = INT;

		if(token == ','){
			match(',');
		}
	}//end while
}

//1.enum_declaration
//2.variable_declaration
//3.function_declaration
void global_declaration(){
	
	int type;

	//1.enum [id] { id [= num] {, id[= num]}}
	if(token == Enum){
		match(Enum);
		if(token != '{'){
			match(Id);	//skip id
		}

		if(token == '{'){
			match('{');
			enum_declaration();
			match('}');
		}

		match(';');
		return;
	}//end token == Enum
	else {
		//maybe variable declaration or function declaration.
		basetype = INT;

		if(token == Int){
			match(Int);
		} else if(token == Char){
			match(Char);
			basetype = CHAR;
		}

		while(token != ';' && token != '}'){
		
			type = basetype;

			//maybe pointer type
			while(token == Mul){
				match(Mul);
				type = type + PTR;
			}

			if(token != Id){
				printf("unvaliable declaration(id) at line:%d\n",line);
				exit(-1);
			}

			if(current_id[Class]){
				//already exist
				printf("unvaliable declaration(already exist) at line:%d",line);
				exit(-1);
			}

			match(Id);
			//set id type 
			current_id[Type] = type;

			if(token == '('){
				//function declaration
				current_id[Class] = Fun;
				current_id[Value] = (int)(text + 1);
				function_declaration();
			} else {
				//variable declaration
				current_id[Class] = Glo;
				current_id[Value] = (int)data;
				data = data + sizeof(int);
			}

			if(token == ','){
				match(',');
			}

		}//end while

		//remove ';' or '}'
		next();
	}//end else

}

void program(){
	//get first token
	next();
	while(token > 0){
		global_declaration();	
	}
}

int eval(){
	int op;
	int *tmp;
	int i;
	while(1){
		op = *pc++;
		if(op == LEA){
			printf("LEA\n");
			printf("%d\n", *pc);
			ax = (int)(bp + *pc++);	//load address of arguments
		}
		else if(op == IMM){
			printf("IMM\n");	
			printf("%d\n", *pc);
			ax = *pc++;				//load immediate into ax
		}
		else if(op == JMP){
			printf("JMP\n");
			pc = (int *)*pc;		//jump the address
		}
		else if(op == CALL){
			printf("CALL\n");
			*--sp = (int)(pc + 1);	//function call
			pc = (int *)*pc;
		}
		else if(op == JZ){
			printf("JZ\n");
			pc = ax ? pc + 1 : (int *)*pc;	//if ax is zero, jump
		}
		else if(op == JNZ){
			printf("JNZ\n");
			printf("%d\n", *pc);

			pc = ax ? (int *)*pc : pc + 1;	//if ax is not zero, jump
		}
		else if(op == ENT){
			printf("ENT\n");
			printf("%d\n", *pc);
			*--sp = (int)bp;	//new stack frame
			bp = sp;
			sp = sp - *pc++;
		}
		else if(op == ADJ){
			printf("ADJ\n");
			printf("%d\n", *pc);
			sp = sp + *pc++;	//remove arguments
		}
		else if(op == LEV){
			printf("LEV\n");
			sp = bp;			//return from function call
			bp = (int *)*sp++;
			pc = (int *)*sp++;
		}
		else if(op == LI){
			printf("LI\n");
			ax = *(int *)ax;	//load integer into ax, address in ax.
		}
		else if(op == LC){
			printf("LC\n");
			ax = *(char *)ax;	//load character into ax, address in ax.
		}
		else if(op == SI){
			printf("SI\n");
			*(int *)*sp++ = ax;	//save integer that in the ax in to the address store in stack
		}
		else if(op == SC){
			printf("SC\n");
			*(char *)*sp++ = ax;	//save character that in the ax to the address stored in stack
		}
		else if(op == PUSH){
			printf("PUSH\n");
			*--sp = ax;			//store the value in ax to stack
		}


		else if(op == OR){
			printf("OR\n");
			ax = *sp++ | ax;	// |
		}
		else if(op == XOR){
			printf("XOR\n");
			ax = *sp++ ^ ax;	// ^
		}
		else if(op == AND){
			printf("AND\n");
			ax = *sp++ & ax;
		}
		else if(op == EQ){
			printf("EQ\n");
			ax = *sp++ == ax;
		}
		else if(op == NE){
			printf("NE\n");
			ax = *sp++ != ax;
		}
		else if(op == LT){
			printf("LT\n");
			ax = *sp++ < ax;
		}
		else if(op == GT){
			printf("GT\n");
			ax = *sp++ > ax;
		}
		else if(op == LE){
			printf("LE\n");
			ax = *sp++ <= ax;
		}
		else if(op == GE){
			printf("GE\n");
			ax = *sp++ >= ax;
		}
		else if(op == SHL){
			printf("SHL\n");
			ax = *sp++ << ax;
		}
		else if(op == SHR){
			printf("SHR\n");
			ax = *sp++ >> ax;
		}
		else if(op == ADD){
			printf("ADD\n");
			ax = *sp++ + ax;
		}
		else if(op == SUB){
			printf("SUB\n");
			ax = *sp++ - ax;
		}
		else if(op == MUL){
			printf("MUL\n");
			ax = *sp++ * ax;
		}
		else if(op == DIV){
			printf("DIV\n");
			ax = *sp++ / ax;
		}
		else if(op == MOD){
			printf("MOD\n");
			ax = *sp++ % ax;
		}


		else if(op == OPEN){
			printf("OPEN\n");
			ax = open((char *)sp[1], sp[0]);	//open file
		}
		else if(op == READ){
			printf("READ\n");
			ax = read(sp[2], (char *)sp[1], sp[0]);	//read file
		}
		else if(op == CLOS){
			printf("CLOS\n");
			ax = close(sp[0]);	//close file
		}
		else if(op == PRTF){
			printf("PRTF\n");
			tmp = sp + pc[1];
			ax = printf((char *)tmp[-1], tmp[-2], tmp[-3], tmp[-4], tmp[-5], tmp[-6]);
		}
		else if(op == MALC){
			printf("MALC\n");
			ax = (int)malloc(sp[0]);	//molloc memory
		}
		else if(op == MSET){
			printf("MSET\n");
			ax = (int)memset((char *)sp[2], sp[1], sp[0]);	//memset
		}
		else if(op == MCMP){
			printf("MCMP\n");
			ax = memcmp((char *)sp[2],(char *)sp[1], sp[0]);
		}
		else if(op == EXIT){
			printf("EXIT\n");
			printf("exit(%d)\n",*sp);
			exit(*sp);
		}
		else{
			printf("unknow command:%d\n",op);
			return -1;
		}
	}//end while(1)
}


int main(int argc, char **argv){
	
	int fd;	//read source file
	int i;	
	int *tmp;

	poolsize= 256 * 1024;

	if(!(fd = open(argv[1], O_RDONLY))){
		printf("can not open source file:%s\n",argv[1]);
		exit(-1);
	}

	if(!(text = (int *)malloc(poolsize))){
		printf("can not malloc for text\n");
		exit(-1);
	}

	if(!(stack = (int *)malloc(poolsize))){
		printf("can not malloc for stack\n");
		exit(-1);
	}

	if(!(data = (char *)malloc(poolsize))){
		printf("can not malloc for data\n");
		exit(-1);
	}

	if(!(symbols = (int *)malloc(poolsize))){
		printf("can not malloc for symbols\n");
		exit(-1);
	}

	memset(text, 0, poolsize);
	memset(stack, 0, poolsize);
	memset(data, 0, poolsize);
	memset(symbols, 0, poolsize);

	sp = bp =(int *)((int)stack + poolsize);
	ax = 0;
	line = 1;

	src = "char else enum if int return sizeof while open read close printf malloc memset memcmp exit void main";

	//add keyword to symbol table
	i = Char;
	while(i <= While){
		next();
		current_id[Token] = i++;

	}

	//add system function to symbol table
	i = OPEN;
	while(i <= EXIT){
		next();
		current_id[Type] = INT;
		current_id[Class] = Sys;
		current_id[Value] = i++;
	}

	next();	//store void to symbol table
	current_id[Token] = Char;
	next();	//store main to symbol table
	idmain = current_id;

	if(!(src = malloc(poolsize))){
		printf("can not malloc for src\n");
		exit(-1);
	}

	//read source file
	if((i = read(fd, src, poolsize - 1)) <= 0){
		printf("read source file error\n");
		return -1;
	}
	src[i] = 0;	//add EOF character
	close(fd);

	program();

	if(!(pc = (int *)idmain[Value])){
		printf("main not defined\n");
		return -1;
	}

	//setup stack
	*--sp = EXIT;	//call exit if main returns
	*--sp = PUSH;
	tmp = sp;
	*--sp = --argc;
	*--sp = (int)(++argv);
	*--sp = (int)tmp;

	return eval();
}














