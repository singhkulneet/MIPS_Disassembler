#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <list>

using namespace std;

//Function used to get the opcode from the instruction
unsigned int getOp( unsigned int integer )
{
	unsigned int bitmask = 4227858432; //0xFC000000
	//retVal only get the 6 msbs
	unsigned int retVal = integer & bitmask;
	//Right shifted by 26 bits to bring the 6 mbs to the begining of the unsigned int
	retVal = retVal >> 26;
	return retVal;
}

//Function used to get the shift amount for R-type instructions that use it
unsigned int getShift( unsigned int integer )
{
	//Important bits are masked then shifted right
	unsigned int bitmask = 1984; //0x000007C0
	unsigned int retVal = integer & bitmask;
	retVal = retVal >> 6;
	return retVal;
}

//This function is used to get the function code for R-type instructions
unsigned int getFuct( unsigned int integer )
{
	//Important bits are masked then shifted right
	unsigned int bitmask = 63; //0x0000003F
	unsigned int retVal = integer & bitmask;
	return retVal;
}

//This function is used to get signed immediate values from I-Type instructions
int getImm( unsigned int integer )
{
	//Important bits are masked then the 16th bit is checked and if neccessary retVal is sign extended
	unsigned int bitmask = 65535; //0x0000FFFF
	int retVal = integer & bitmask;
	unsigned int bitmask2 = 32768;
	if( (bitmask2 & retVal) == bitmask2 )
	{
		retVal = retVal | 4294901760; //0xFFFF0000
	}
	return retVal;
}

//This function is used to get unsigned immediate values from I-Type instructions
unsigned int getUImm( unsigned int integer )
{
	//Important bits are masked then returned
	unsigned int bitmask = 65535; //0x0000FFFF
	unsigned int retVal = integer & bitmask;
	return retVal;
}

//This is a helper function for the getRX functions to get the string for each register type
string getReg(unsigned int val)
{
	//Switch case that test the passed in value and returns proper string for corresponding register
	switch(val)
	{
		case 0:
			return "$zero";
		break;
		
		case 1:
			return "$at";
		break;
		
		case 2:
			return "$v0";
		break;
		
		case 3:
			return "$v1";
		break;
		
		case 4:
			return "$a0";
		break;
		
		case 5:
			return "$a1";
		break;
		
		case 6:
			return "$a2";
		break;
		
		case 7:
			return "$a3";
		break;
		
		case 8:
			return "$t0";
		break;
		
		case 9:
			return "$t1";
		break;
		
		case 10:
			return "$t2";
		break;
		
		case 11:
			return "$t3";
		break;
		
		case 12:
			return "$t4";
		break;
		
		case 13:
			return "$t5";
		break;
		
		case 14:
			return "$t6";
		break;
		
		case 15:
			return "$t7";
		break;
		
		case 16:
			return "$s0";
		break;
		
		case 17:
			return "$s1";
		break;
		
		case 18:
			return "$s2";
		break;
		
		case 19:
			return "$s3";
		break;
		
		case 20:
			return "$s4";
		break;
		
		case 21:
			return "$s5";
		break;
		
		case 22:
			return "$s6";
		break;
		
		case 23:
			return "$s7";
		break;
		
		case 24:
			return "$t8";
		break;
		
		case 25:
			return "$t9";
		break;
		
		default:
			return "invalid";
	}
}

//This function is used to get the string equivalent of the RD register
string getRd( unsigned int integer)
{
	//Important bits are masked and shifted right
	unsigned int bitmask = 63488; //0x0000F800
	unsigned int retVal = integer & bitmask;
	retVal = retVal >> 11;
	//Retval is sent to the getReg helper function to get the string for the register
	return getReg(retVal);
}

//This function is used to get the string equivalent of the Rs register
string getRs( unsigned int integer)
{
	//Important bits are masked and shifted right
	unsigned int bitmask = 65011712; //0x03E00000
	unsigned int retVal = integer & bitmask;
	retVal = retVal >> 21;
	//Retval is sent to the getReg helper function to get the string for the register
	return getReg(retVal);
}

//This function is used to get the string equivalent of the Rt register
string getRt( unsigned int integer)
{
	//Important bits are masked and shifted right
	unsigned int bitmask = 2031616; //0x001F0000
	unsigned int retVal = integer & bitmask;
	retVal = retVal >> 16;
	//Retval is sent to the getReg helper function to get the string for the register
	return getReg(retVal);
}

//Helper function to get the hex of an address for lables
string getAddress(int address)
{
	//Hex conversion
	stringstream ss;
	ss << hex << address;
	string retVal(ss.str()); 
	//Padding with 0's based on string size
	if(retVal.size() == 1)
	{
		retVal = "000" + retVal;
	}
	else if(retVal.size() == 2)
	{
		retVal = "00" + retVal;
	}
	else if(retVal.size() == 3)
	{
		retVal = "0" + retVal;
	}
	return retVal;
}

//This is the main function used to disassemble instructions given in hex and convert them to MIPS assembly code
int main(int argc, char *argv[])
{
	//Declaring a variable to test if there is an error in the .asm file
	bool error = false;
	
	//Testing if the right number of arguments are passed to the function
	if(argc != 2)
	{
		cout << "Wrong number of command line arguments, try again";
		return 0;
	}
	
	//Making a temporary string for testing purposes
	string temp = argv[1];
	
	//Testing if the value passed to the function is a .obj file
	if( temp.substr(temp.size()-4, 4) != ".obj")
	{
		cout << "Wrong file extension";
		return 0;
	}
	
	
	//Creating an input and output filestream for file reading/writing 
	ifstream in2(argv[1]);
	string output = temp.substr(0, temp.size()-3) + "s";
	ofstream out(output);
	
	//Declaring necessary variable needed to properly decode instructions
	string input;
	unsigned int integer = 0;
	int line = 1;
	stringstream ss;
	stringstream ss1;
	int opcode;
	int funct;
	int address;
	char buf[] = "0000";
	string adOut = "0000";
	
	/*	
	*	first pass used to test for branch instructions store lines necessary to 
	*	output addresses in the second pass.
	*/
	
	//geting the first instruction in hex
	in2 >> input;
	//error check to make sure the size of the instruction is exactly 32 bits
	if( input.size() != 8 )
	{
		cout << "Cannot disassemble " << input << " at line " << line;
		error = true;
	}
	
	//declaring a list to hold all of the lines that are branch targets
	list<int> lineList;
	
	//While loop that goes through the whole file to look for branch instructions
	//and store the target in a list.
	while(!in2.fail())
	{
		//Converting hex to unsigned int
		ss << hex << input;
		ss >> integer;
		ss.clear();
		//getting the opcode from the instruction
		opcode = getOp(integer);
		//when a branch is found the branch target is stored in the list
		if((opcode == 5) || (opcode == 4))
		{
			lineList.push_back(line + getImm(integer));
		}	
		//Incrementing the line number
		line++;
		//getting next instruction + error checking
		in2 >> input;
		if( input.size() != 8 )
		{
			cout << "Cannot disassemble " << input << " at line " << line;
			error = true;
		}
	}
	//sorting the list of target lines in numerical order
	//Then calling the unique method to make sure there are no duplicates
	lineList.sort();
	lineList.unique();
	//resetting line number and closeing the current filestream
	line = 1;
	in2.close();
	//openning a new ifstream for second pass.
	ifstream in(argv[1]);
	//Second Pass 
	//Taking in first instruction from file
	in >> input;
	if( input.size() != 8 )
	{
		cout << "Cannot disassemble " << input << " at line " << line;
		error = true;
	}
	//While loop to read all instructions in the file
	while(!in.fail())
	{
		//For each line checking the list to see if it is a branch target, if it is a lable is placed on the ouput file
		for (std::list<int>::iterator it=lineList.begin(); it != lineList.end(); ++it)
		{
			if( *it == line-1)
			{
				address = (line-1) * 4;
				adOut = getAddress(address);
				out << "Addr_" << adOut << ":" << endl;
			}
		}
		//Converting hex instruction to 32 bit integer instruction
		ss << hex << input;
		ss >> integer;
		ss.clear();
		//Getting opcode from instruction
		opcode = getOp(integer);
		//Switch case to choose instruction based on opcode
		switch(opcode) 
		{
			//Case for R-type instructions
			case 0:	
				//Getting fuction code from instruction
				funct = getFuct(integer);
				//Switch case for R-type instructions based on function code to find appropriate operation
				switch(funct)
				{
					case 0:
						out << "\tsll " << getRd(integer) << ", " << getRt(integer) << ", " << getShift(integer) << endl;
					break;
					
					case 2:
						out << "\tsrl " << getRd(integer) << ", " << getRt(integer) << ", " << getShift(integer) << endl;
					break;
					
					case 32:
						out << "\tadd " << getRd(integer) << ", " << getRs(integer) << ", " << getRt(integer) << endl;
					break;		

					case 33:
						out << "\taddu " << getRd(integer) << ", " << getRs(integer) << ", " << getRt(integer) << endl;
					break;
					
					case 34:
						out << "\tsub " << getRd(integer) << ", " << getRs(integer) << ", " << getRt(integer) << endl;
					break;
					
					case 35:
						out << "\tsubu " << getRd(integer) << ", " << getRs(integer) << ", " << getRt(integer) << endl;
					break;
					
					case 36:
						out << "\tand " << getRd(integer) << ", " << getRs(integer) << ", " << getRt(integer) << endl;
					break;
					
					case 37:
						out << "\tor " << getRd(integer) << ", " << getRs(integer) << ", " << getRt(integer) << endl;
					break;
					
					case 39:
						out << "\tnor " << getRd(integer) << ", " << getRs(integer) << ", " << getRt(integer) << endl;
					break;
					
					case 42:
						out << "\tslt " << getRd(integer) << ", " << getRs(integer) << ", " << getRt(integer) << endl;
					break;
					
					case 43:
						out << "\tsltu " << getRd(integer) << ", " << getRs(integer) << ", " << getRt(integer) << endl;
					break;
					//If none of the known instruction are found an error message is printed 
					//and an error flag is generated 
					default:
						cout << "Cannot disassemble " << input << " at line " << line;
						error = true;
				}
			break;
			//Case for branch on equal instruction
			case 4:
				out << "\tbeq " << getRs(integer) << ", " << getRt(integer) << ", ";
				address = (line + getImm(integer)) * 4;
				adOut = getAddress(address);
				out << "Addr_" << adOut << endl;
			break;
			//case for branch not equal instruction
			case 5:
				out << "\tbne " << getRs(integer) << ", " << getRt(integer) << ", ";
				address = (line + getImm(integer)) * 4;
				adOut = getAddress(address);
				out << "Addr_" << adOut << endl;
			break;
			//Cases for various other instructions below
			case 8:
				out << "\taddi " << getRt(integer) << ", " << getRs(integer) << ", " << getImm(integer) << endl;
			break;
			
			case 9:
				out << "\taddiu " << getRt(integer) << ", " << getRs(integer) << ", " << getImm(integer) << endl;
			break;
			
			case 12:
				out << "\tandi " << getRt(integer) << ", " << getRs(integer) << ", " << getUImm(integer) << endl;
			break;
			
			case 36:
				out << "\tlbu " << getRt(integer) << ", " <<  getImm(integer) << "(" << getRs(integer) << ")" << endl;
			break;
			
			case 37:
				out << "\tlhu " << getRt(integer) << ", " <<  getImm(integer) << "(" << getRs(integer) << ")" << endl;
			break;
			
			case 48:
				out << "\tll " << getRt(integer) << ", " <<  getImm(integer) << "(" << getRs(integer) << ")" << endl;
			break;
			
			case 15:
				out << "\tlui " << getRt(integer) << ", " <<  getImm(integer) << endl;
			break;
			
			case 35:
				out << "\tlw " << getRt(integer) << ", " <<  getImm(integer) << "(" << getRs(integer) << ")" << endl;
			break;
			
			case 13:
				out << "\tori " << getRt(integer) << ", " << getRs(integer) << ", " << getImm(integer) << endl;
			break;
			
			case 10:
				out << "\tslti " << getRt(integer) << ", " << getRs(integer) << ", " << getImm(integer) << endl;
			break;
			
			case 11:
				out << "\tsltiu " << getRt(integer) << ", " << getRs(integer) << ", " << getImm(integer) << endl;
			break;
			
			case 40:
				out << "\tsb " << getRt(integer) << ", " <<  getImm(integer) << "(" << getRs(integer) << ")" << endl;
			break;
			
			case 41:
				out << "\tsh " << getRt(integer) << ", " <<  getImm(integer) << "(" << getRs(integer) << ")" << endl;
			break;
			
			case 43:
				out << "\tsw " << getRt(integer) << ", " <<  getImm(integer) << "(" << getRs(integer) << ")" << endl;
			break;
			
			case 56:
				out << "\tsc " << getRt(integer) << ", " <<  getImm(integer) << "(" << getRs(integer) << ")" << endl;
			break;		
			//If none of the known instruction are found an error message is printed 
			//and an error flag is generated
			default:
				cout << "Cannot disassemble " << input << " at line " << line; 
				error = true;
		}	
		//line nuber is incremented 
		line++;
		in >> input;
		//if input is not a 32 bit hex instructions error is returned
		if( input.size() != 8 )
		{
			cout << "Cannot disassemble " << input << " at line " << line;
			error = true;
		}		
	}	
	//If error flag was triggered then the output file is deleted else the output file is closed
	if(error)
	{
		out.close();
		remove(output.c_str());
	}
	else
	{
		out.close();
	}
	//inputfile is closed
	in.close();
	//end of program, 0 is returned
	return 0;
}