using namespace std;

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>

bool IsOneChar(const string &str) {
	return (str.compare("+") == 0) || (str.compare(":") == 0) || (str.compare(",") == 0) || (str.compare("[") == 0) || (str.compare("]") == 0);
}

bool IsReg8(const string &str) {
	return (str.compare("AH") == 0) || (str.compare("BH") == 0) || (str.compare("CH") == 0) || (str.compare("DH") == 0) || (str.compare("AL") == 0) || (str.compare("BL") == 0) || (str.compare("CL") == 0) || (str.compare("DL") == 0);
}

bool IsReg16(const string &str) {
	return (str.compare("AX") == 0) || (str.compare("BX") == 0) || (str.compare("CX") == 0) || (str.compare("DX") == 0) || (str.compare("SI") == 0) || (str.compare("DI") == 0) || (str.compare("SP") == 0) || (str.compare("BP") == 0);
}

bool IsSReg(const string &str) {
	return (str.compare("CS") == 0) || (str.compare("DS") == 0) || (str.compare("SS") == 0) || (str.compare("ES") == 0) || (str.compare("FS") == 0) || (str.compare("GS") == 0);
}

bool IsDataType(const string &str) {
	return (str.compare("DB") == 0) || (str.compare("DW") == 0);
}

bool IsPtrType(const string &str) {
	return (str.compare("FAR") == 0) || (str.compare("BYTE") == 0) || (str.compare("DWORD") == 0);
}

bool IsPtrPtr(const string &str) {
	return str.compare("PTR") == 0;
}

bool IsDirective(const string &str) {
	return (str.compare("END") == 0) || (str.compare("SEGMENT") == 0) || (str.compare("ENDS") == 0) || (str.compare("PROC") == 0) || (str.compare("ENDP") == 0) || (str.compare("ASSUME") == 0);
}

bool IsCommand(const string &str) {
	return (str.compare("RET") == 0) || (str.compare("NOT") == 0) || (str.compare("OR") == 0) || (str.compare("ADD") == 0) || (str.compare("MOV") == 0) || (str.compare("JGE") == 0) || (str.compare("CALL") == 0);
}

bool IsNumber(const string &str) {
	return isdigit(str.front());
}

bool IsString(const string &str) {
	return str.front() == '\'';
}

bool IsReserved(const string &str) {
	return IsReg8(str) || IsReg16(str) || IsSReg(str) || IsDataType(str) || IsPtrType(str) || IsPtrPtr(str) || IsDirective(str) || IsCommand(str);
}

bool IsIdentifier(const string &str) {
	return isalpha(str.front()) && !IsReserved(str);
}

string getLexemInfo(const string &str) {
	if (IsString(str)) return "string";
	else if (IsNumber(str)) return "number";
	else if (IsString(str)) return "string";
	else if (IsOneChar(str)) return "one char";
	else if (IsDataType(str)) return "data type";
	else if (IsPtrType(str)) return "ptr type";
	else if (IsPtrPtr(str)) return "ptr operator";
	else if (IsReg8(str)) return "8-bit register";
	else if (IsReg16(str)) return "16-bit register";
	else if (IsSReg(str)) return "segment register";
	else if (IsCommand(str)) return "command";
	else if (IsDirective(str)) return "directive";
	else if (IsIdentifier(str)) return "identifier";
	else return "unknown";
}

vector<string> divide(const string &str) {
	vector<string> lexems;
	int i = 0, index = 0;
	while (i < str.size()) {
		if (str[i] == ';') break;
		else if (isspace(str[i])) i++;
		else {
			string text;
			if (isalpha(str[i])) {
				while ((i < str.size()) && isalnum(str[i])) {
					text += toupper(str[i++]);
				}
			} else if (isdigit(str[i])) {
				while ((i < str.size()) && isxdigit(str[i])) {
					text += toupper(str[i++]);
				}
				if (toupper(str[i]) == 'H') {
					text += toupper(str[i++]);
				}
			} else if (str[i] == '\'') {
				text += str[i++];
				while ((i < str.size()) && (str[i] != '\'')) {
					text += str[i++];
				}
				text += str[i++];
			} else if (!IsOneChar(text = str[i++])) {
				text = "";
			}

			lexems.push_back(text);
		}
	}
	return lexems;
}

struct Symbol {
	string type, segment, attr;
	unsigned offset;
};

struct Operand {
	vector<string> lexems;

	struct Table {
		bool present;
		string first, second;
	} table[10];

	Operand(const vector<string> &lexems) {
		this->lexems = lexems;

		int i = 0;
		if ((i < lexems.size()) && IsPtrType(lexems[i])) {
			string ptr = lexems[i++];
			if ((i < lexems.size()) && IsPtrPtr(lexems[i])) {
				i++;
				if (ptr.compare("FAR") == 0) {
					this->ptr = -2;
				} else if (ptr.compare("BYTE") == 0) {
					this->ptr = 1;
				} else if (ptr.compare("WORD") == 0) {
					this->ptr = 2;
				}
			}
		}

		if (i < lexems.size()) {
			if (IsReg8(lexems[i]) || IsReg16(lexems[i])) {
				reg = lexems[i++];
			} else if (IsNumber(lexems[i])) {
				imm = stol(lexems[i++], 0, 16);
			} else if (IsString(lexems[i])) {
				str = lexems[i++];
			}
		}

		if ((i < lexems.size()) && IsSReg(lexems[i])) {
			string sreg = lexems[i++];
			if ((i < lexems.size()) && lexems[i].compare(":") == 0) {
				i++;
				sreg = sreg;
			}
		}

		if ((i < lexems.size()) && IsIdentifier(lexems[i])) {
			ident = lexems[i++];
		}

		if ((i < lexems.size()) && (lexems[i].compare("[") == 0)) {
			i++;
			if ((i < lexems.size()) && IsReg16(lexems[i])) {
				base = lexems[i++];
				if ((i < lexems.size()) && (lexems[i].compare("+") == 0)) {
					i++;
					if ((i < lexems.size()) && IsReg16(lexems[i])) {
						index = lexems[i++];
						scale = 1;
						if ((i < lexems.size()) && (lexems[i].compare("+") == 0)) {
							i++;
							if ((i < lexems.size()) && IsNumber(lexems[i])) {
								disp = stol(lexems[i++], 0, 16);
							}
						}
					}
				}
			}
			if ((i < lexems.size()) && (lexems[i].compare("]") == 0)) {
				i++;
			}
		}
	}
};

struct Sentence {
	string label, ident, mnemo;
	vector<Operand> operands;
	unsigned length, offset;
	vector<string> lexems;
	string text, bytes;
	bool printed;

	Sentence(const string &text) {	
		this->text = text;
		this->lexems = divide(text);
		this->printed = false;
		this->length = 0;
		this->offset = 0;

		int i = 0;
		if ((i < lexems.size()) && IsIdentifier(lexems[i])) {
			string str = lexems[i++];
			if ((i < lexems.size()) && (lexems[i].compare(":") == 0)) {
				label = str;
				i++;
			} else ident = str;
		}

		if ((i < lexems.size()) && (IsDirective(lexems[i]) || IsCommand(lexems[i]) || IsDataType(lexems[i]))) {
			mnemo = lexems[i++];
		}

		while ((i < lexems.size())) {
			vector<string> operand;
			while (i < lexems.size()) {
				if (lexems[i].compare(",") == 0) {
					i++; 
					break;
				}
				operand.push_back(lexems[i++]);
			}
			operands.push_back(Operand(operand));
		}
	}

	void Print(FILE *file) {
		if (text.empty()) return;
		fprintf(file, "text: %s\n", text.c_str());

		fprintf(file, " Label  Mnemocode  1st operand  2nd operand\n");
		fprintf(file, " index    index    index count  index count\n");

		int index = 0;
		int label_index = (IsIdentifier(label) || IsIdentifier(ident)) ? ++index : -1;
		int mnemo_index = (IsDirective(mnemo) || IsCommand(mnemo) || IsDataType(mnemo))  ? ++index : -1;
		int op1_index = (operands.size() > 0) ? ++index : -1;
		int op1_count = (operands.size() > 0) ? operands[0].lexems.size() : 0;
		int op2_index = (operands.size() > 1) ? (op1_index + op1_count) : -1;
		int op2_count = (operands.size() > 1) ? operands[1].lexems.size() : 0;
		fprintf(file, " %5i  %9i  %5i %5i  %5i %5i\n\n", label_index, mnemo_index, op1_index, op1_count, op2_index, op2_count);	
		index = 0;
		for (string &lexem : lexems) {
			fprintf(file, "%-2d | %11s | %2i | %16s |\n", ++index, lexem.c_str(), lexem.size(), getLexemInfo(lexem).c_str());
		}
		fprintf(file, "\n");
	}
};

struct View {
	map<string, unsigned> segmentsTable;
	map<string, Symbol> symbolsTable;
	map<string, string> assume_table;
	string segment, fileName, ds;
	vector<Sentence> sentences;
	unsigned offset;

	void Parse(const string &);
	void PrintSentences(FILE * = NULL);
	void PrintListing(FILE * = NULL);
	void GenerateListing();
};

void View::Parse(const string &path) {
	size_t index = path.find_last_of(".");
	if (index != -1) {
		fileName = path.substr(0, index);
	} else {
		fileName = path;
	}

	ifstream file(path);
	ds = "DS";

	string line;
	while (getline(file, line)) {
		Sentence sentence(line);
		
		if (!sentence.label.empty()) {
			Symbol symbol;
			symbol.type = "L NEAR";
			symbol.offset = offset;
			symbol.segment = segment;
			symbolsTable[sentence.label] = symbol;
			sentence.printed = true;
		}

		if (!sentence.mnemo.empty()) {
			if (sentence.mnemo.compare("END") == 0) {

			} else if (sentence.mnemo.compare("SEGMENT") == 0) {
				sentence.printed = true;
				segment = sentence.ident;
				offset = 0;
			} else if (sentence.mnemo.compare("ENDS") == 0) {
				sentence.printed = true;
				segmentsTable[segment] = offset;
				segment.clear();
			} else if (sentence.mnemo.compare("PROC") == 0) {
				sentence.printed = true;
				Symbol symbol;

				symbol.type = "N PROC";
				if (!sentence.operands.empty()) {
					if (sentence.operands[0].lexems[0].compare("FAR") == 0) {
						symbol.type = "F PROC";
					}
				}
				symbol.offset = offset;
				symbol.segment = segment;
				if (!sentence.ident.empty()) {
					symbolsTable[sentence.ident] = symbol;
				}
			} else if (sentence.mnemo.compare("ENDP") == 0) {
				sentence.printed = true;
				if (!sentence.ident.empty()) {
					char buf[14] = {0};
					sprintf(buf, "Length = %.4X", offset - symbolsTable[sentence.ident].offset);
					symbolsTable[sentence.ident].attr = '\t' + string(buf);
				}
			} else if (sentence.mnemo.compare("ASSUME") == 0) {
				for (Operand &operand : sentence.operands) {
					assume_table[operand.Get(2).first] = operand.Get(4).first;
				}
			} else if (IsDataType(sentence.mnemo)) {
				sentence.printed = true;

				Symbol symbol;
				symbol.offset = offset;
				symbol.segment = segment;

				if (sentence.mnemo.compare("DB") == 0) {
					symbol.type = "L BYTE";
					if (IsString(sentence.operands[0].Get(9).first)) {
						sentence.length = sentence.operands[0].Get(9).first.size() - 2;
					} else sentence.length = 1; 
				} else if (sentence.mnemo.compare("DW") == 0) {
					symbol.type = "L WORD";
					sentence.length = 2;
				}

				if (!sentence.ident.empty())
					symbolsTable[sentence.ident] = symbol;
			} else {
				sentence.printed = true;

				if (sentence.mnemo.compare("RET") == 0) {
					sentence.length = 1;
				} else if (sentence.mnemo.compare("NOT") == 0) {
					if (sentence.operands.size() > 0) {
						if (sentence.operands[0].IsRegister()) {
							sentence.length = 2;
						}
					}
				} else if (sentence.mnemo.compare("OR") == 0) {
					if (sentence.operands.size() > 1) {
						if (sentence.operands[0].IsRegister() && sentence.operands[1].IsMemory()) {
							 sentence.length = 2;

							if (sentence.operands[1].Get(8).present) {
								long number = stol(sentence.operands[1].Get(8).first, 0, 16);
								if (number != 0) {
									if ((number >= -128) && (number <= 127)) {
										sentence.length += 1;
									} else {
										sentence.length += 4;
									}
								}
							}

							if (sentence.operands[1].Get(2).present) {
								if (sentence.operands[1].Get(2).first.compare("ds") != 0) {
									sentence.length += 1;
								}
							}

							if (sentence.operands[1].table[6].present) {
								sentence.length += 1;
							}
						}
					}
				} else if (sentence.mnemo.compare("ADD") == 0) {
					if (sentence.operands.size() > 0) {
						if (sentence.operands[0].IsRegister() && sentence.operands[1].IsRegister()) {
							sentence.length = 2;
						}
					}
				} else if (sentence.mnemo.compare("MOV") == 0) {
					if (sentence.operands.size() > 1) {
						if (sentence.operands[0].IsMemory() && sentence.operands[1].IsImmediate()) {
							sentence.length = 2;

							if (sentence.operands[0].table[3].present || sentence.operands[0].table[4].present) {
								sentence.length += 2;
							} else {
								sentence.length += 1;
							}

							if ((sentence.operands[0].Get(1).present) && (sentence.operands[0].Get(1).first.compare("1") == 0)) {
								sentence.length += 1;
							} else {
								sentence.length += 2;
							}

							if (sentence.operands[0].Get(2).present) {
								if (sentence.operands[0].Get(2).first.compare("ds") != 0) {
									sentence.length += 1;
								}
							} else if (sentence.operands[0].table[3].present) {
								if (segment.compare(symbolsTable.find(sentence.operands[0].Get(3).first)->second.segment) != 0) {
									sentence.length += 1;
								}
							}
						}
					}
				} else if (sentence.mnemo.compare("JGE") == 0) {
					if (sentence.operands.size() > 0) {
						if (sentence.operands[0].IsIdentifier()) {
							if (sentence.operands[0].Get(3).present) {
								Symbol symbol = symbolsTable.find(sentence.operands[0].Get(3).first)->second;
								sentence.length = (offset - symbol.offset) > 0xFF ? 4 : 2;
							} else {
								sentence.length = 4;
							}
						}
					}
				} else if (sentence.mnemo.compare("CALL") == 0) {
					if (sentence.operands.size() > 0) {
						if (sentence.operands[0].IsRegister()) {
							sentence.length = 2;
						} else if (sentence.operands[0].IsMemory()) {
							if (sentence.operands[0].Get(3).present) {
								Symbol symbol = symbolsTable.find(sentence.operands[0].Get(3).first)->second;
								
								if (symbol.type.compare("F PROC") == 0) {
									sentence.length = 5;
								} else if (symbol.type.compare("N PROC") == 0) {
									if ((sentence.operands[0].Get(1).present) && (sentence.operands[0].Get(1).first.compare("-2") == 0)) {
										sentence.length = 5;
									} else {
										sentence.length = 3;
									}
								} else {
									if ((sentence.operands[0].Get(1).present) && (sentence.operands[0].Get(1).first.compare("-2") == 0)) {
										sentence.length = 4;

										if (sentence.operands[0].Get(2).present) {
											sentence.length += 1;
										}
									} else {
										sentence.length = 5;
									}
								}
							}
						}
					} else {
						sentence.length = 4;
					}
				}
			}
		}
		sentence.offset = offset;
		offset += sentence.length;
		sentences.push_back(sentence);
	}
	file.close();
}

void View::PrintSentences(FILE *file) {
	bool close = false;
	if (file == NULL) {
		close = true;
		file = fopen((fileName + "_sentences.txt").c_str(), "w");
	}
	if (file) {
		for (Sentence &sentence : sentences) {
			sentence.Print(file);
		}
	}
	if (close) {
		fclose(file);
	}
}

void View::PrintListing(FILE *file) {
	bool close = false;
	if (file == NULL) {
		close = true;
		file = fopen((fileName + ".lst").c_str(), "w");
	}
	for (Sentence &sentence : sentences) {
		if (sentence.printed) {
			fprintf(file, " %.4X ", sentence.offset);
		} else {
			fprintf(file, "      ");
		}
		fprintf(file, "%s", sentence.bytes.c_str());
		fprintf(file, "\t\t%s\n", sentence.text.c_str());
	}

	fprintf(file, "\n\n                N a m e         	Size	Length\n\n");
	for (auto &segment : segmentsTable) {
		fprintf(file, "%-*s\t%-7s\t%-.4X\n", 32, segment.first.c_str(), "16 Bit", segment.second);
	}
	
	fprintf(file, "\nSymbols:\n                N a m e         	Type	 Value	 Attr\n\n");
	for (auto &symbol : symbolsTable) {
		fprintf(file, "%-*s\t%-7s\t%-.4X\t%s\n", 32, symbol.first.c_str(), symbol.second.type.c_str(), symbol.second.offset, (symbol.second.segment + symbol.second.attr).c_str());
	}

	fprintf(file, "\n@FILENAME                       \tTEXT  %s\n", fileName.c_str());
	if (close) {
		fclose(file);
	}
}

int main() {
	View view;
	view.Parse("test.asm");
	view.PrintSentences(stdout);
	view.PrintSentences();
	view.PrintListing(stdout);
	view.PrintListing();
}
