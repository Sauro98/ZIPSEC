#include "deflate_decoder.h"

Deflate_decoder::Deflate_decoder(std::string _original){
	original = _original;
	long* static_codes = construct_static_literal_and_lengths_tree();
	/*
		EXPLANATION OF THE NEXT FOR LOOP
		
		for every char that has a prefix 
			loop through prefix bits and if
				- the bit is 0 then we move to the left branch of the current node (and create it if necessary)
				- the bit is 1 then we move to the right branch of the current node (and create it if necessary)
				if the end of the prefix is reached then insert the symbol that the current prefix represents in the current node
				move on to the next prefix
			at the end of the loop we are left with a tree that will allow us to loop through the encoded string bits and decode it
			
	*/
	
	static_root = new s_node_f((Static_Huffman_node*)NULL);
    
    for(int c_i = 0;c_i < 288;c_i++){
    	long code = static_codes[c_i];
    	int code_length = get_static_literal_length(c_i);
    	int mask = get_mask_for_length(code_length);
    	s_node_f* current_node = static_root;
    	for(int a = 0;a < code_length; a++){
    		int curr = ((code <<  (a )) & mask ) >> (code_length - 1);
    		if(curr == 0){
    			if(current_node->left_child == NULL)
    				current_node->left_child = new s_node_f(current_node);
    			current_node = current_node->left_child;
    		}else{
    			if(current_node->right_child == NULL)
    				current_node->right_child = new s_node_f(current_node);
    			current_node = current_node->right_child;
    		}
    		if(a == code_length - 1)
    			current_node->value = c_i;
    	}
    }
}

std::string Deflate_decoder::decode(){
	std::ostringstream debug_result;
	//input string pointers
	long current_byte = 0;
	int current_bit = 0;
	
	bool lastwasliteral = true;
	
	//output string
	std::string output = "";
	
	#if DEBUG > 1
		for(int a = 0; a < original.length();a++){
	            for(int b = 0; b < 8;b++){
	                    print(read_bits(&current_byte,&current_bit,1));
	            }
	            println("");
	    }
	    current_byte = 0;
		current_bit = 0;
	#endif
	
	int bfinal = 0;
	do{
        bfinal = read_bits(&current_byte,&current_bit,1);
		int btype = read_bits(&current_byte,&current_bit,2);
		#if DEBUG > 0
			debug_result << "BTYPE: ";
			debug_result << btype;
			debug_result << "\nBFINAL: ";
			debug_result << bfinal;
		#endif
		if(btype == BTYPE00){
            while(current_bit != 8){
                read_bits(&current_byte,&current_bit,1);
            }
            int16_t length = read_bits(&current_byte,&current_bit,16,false);
            int16_t nlength = read_bits(&current_byte,&current_bit,16,false);
            int16_t nl_test = length ^ 65535;
            if(nlength == nl_test){
                println("valid");
                for(int a = 0;a < length;a++){
                    output += (char)read_bits(&current_byte,&current_bit,8);        
                }
            }     
			
		}else if(btype == BTYPE10){
			
		}else if(btype == BTYPE01){
			#if DEBUG > 0
				debug_result << "\nSTATIC DECODING\n";
			#endif
			int decoded_value = 0;
			do{
				decoded_value = get_next_static_value(&current_byte,&current_bit);
				if(decoded_value < 256){
					#if DEBUG > 0
						if(!lastwasliteral)
						debug_result <<"literal '";
						debug_result << (char)decoded_value;
					#endif
					output += (char)decoded_value;
					lastwasliteral = true;
				}else if(decoded_value > 256){
					int length = get_length_from_code(decoded_value);
					int e_b = 0;
					get_static_length_extra_bits(length,&e_b);
					int extra_bits = read_bits(&current_byte,&current_bit,e_b);
					length += extra_bits;
					#if DEBUG > 0
						if(lastwasliteral)
						debug_result << "\n";
						debug_result << "match (";
						debug_result << length;
						
					#endif
					
					//DISTANCES DO NOT HAVE TO BE READ IN REVERSE ORDER SO ADD TRUE WHEN READING A DISTANCE WITH read_bits
					
					int distance_code = read_bits(&current_byte,&current_bit,5,true);
					int distance = get_distance_from_code(distance_code);
					e_b = 0;
					get_static_distance_extra_bits(distance,&e_b);
					extra_bits = read_bits(&current_byte,&current_bit,e_b,false);
					distance += extra_bits;
					
					#if DEBUG > 0
						debug_result << ",";
						debug_result << distance;
						debug_result << ") \n";
					#endif
					
					//println("\n eb "<<extra_bits<<" l "<<e_b);
					int p_l = output.length();
					for(int i = p_l - distance;i < (p_l - distance) + length; i++){
						if(i >= 0){
							output += output.at(i);
						} else print("error");
					}
					std::ofstream myfile;
                    myfile.open ("decompressed_debug.txt");
                    myfile << debug_result.str();
                    myfile << "\n";
                    myfile << output;
                    myfile.close();
					lastwasliteral = false;
		
				}
			}while(decoded_value != END_CODE);
			
		}else{
			#if DEBUG > 0
				println("CRITICAL ERROR");
			#endif
		}
	}while(bfinal != BFINAL);
	
	println("");
	
	#if DEBUG > 1
		println("OUTPUT: "<<output);
	#endif
	
	//println(debug_result);
	
	std::ofstream myfile;
    myfile.open ("decompressed_debug.txt");
    myfile << debug_result.str();
    myfile << "\n";
    myfile << output;
    myfile.close();
	return output;
}

int Deflate_decoder::get_next_static_value(long*current_char,int* current_bit){
	s_node_f* curr_node = static_root;
	while(curr_node->value == -1){
		int movement = read_bits(current_char,current_bit,1);
		//print(movement);
		if(movement == 0)
			curr_node = curr_node->left_child;
		else
			curr_node = curr_node->right_child;
	}
	//println("");
	int value = curr_node->value;
	//println("value "<<value<<" "<<value);
	return value;
}

int Deflate_decoder::get_distance_from_code(int code){
	if(code <= 4)
		return code + 1;
	switch(code){
		case 5:
			return 7;
		case 6:
			return 9;
		case 7:
			return 13;
		case 8:
			return 17;
		case 9:
			return 25;
		case 10:
			return 33;
		case 11:
			return 49;
		case 12:
			return 65;
		case 13:
			return 97;
		case 14:
			return 129;
		case 15:
			return 193;
		case 16:
			return 257;
		case 17:
			return 385;
		case 18:
			return 513;
		case 19:
			return 769;
		case 20:
			return 1025;
		case 21:
			return 1537;
		case 22:
			return 2049;
		case 23:
			return 3073;
		case 24:
			return 4097;
		case 25:
			return 6145;
		case 26:
			return 8193;
		case 27:
			return 12289;
		case 28:
			return 16385;
		case 29:
			return 24577;
		
	}
	return 0;
}

int Deflate_decoder::get_length_from_code(int code){
	switch(code){
		case 257:
			return 3;
		case 258:
			return 4;
		case 259:
			return 5;
		case 260:
			return 6;
		case 261:
			return 7;
		case 262:
			return 8;
		case 263:
			return 9;
		case 264:
			return 10;
		case 265:
			return 11;
		case 266:
			return 13;
		case 267:
			return 15;
		case 268:
			return 17;
		case 269:
			return 19;
		case 270:
			return 23;
		case 271:
			return 27;
		case 272:
			return 31;
		case 273:
			return 35;
		case 274:
			return 43;
		case 275:
			return 51;
		case 276:
			return 59;
		case 277:
			return 67;
		case 278:
			return 83;
		case 279:
			return 99;
		case 280:
			return 115;
		case 281:
			return 131;
		case 282:
			return 163;
		case 283:
			return 195;
		case 284:
			return 227;
		case 285:
			return 258;	
	}
	return 0;
}

unsigned int Deflate_decoder::read_bits(long* current_char,int* current_bit,int bits_count,bool reversed){
	unsigned int to_return = 0;
	char curr_char = original.at(*current_char);
	for(int read_bit = 0;read_bit < bits_count;read_bit++){
		if((*current_bit) == 8){
			(*current_bit) = 0;
			(*current_char)++;
			curr_char = original.at(*current_char);
		}
		unsigned char bit_to_add = (unsigned char)(curr_char << (7 - (*current_bit))) >> 7;
		if(!reversed)
			to_return |= (bit_to_add << (read_bit));
		else
			to_return |= (bit_to_add << (bits_count - 1 - read_bit));
		(*current_bit)++;
	}
	return to_return;
}

unsigned int Deflate_decoder::read_bits(long* current_char,int* current_bit,int bits_count){
	return read_bits(current_char,current_bit,bits_count,false);
}
