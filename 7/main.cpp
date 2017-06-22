using namespace std;

#include <iostream>
#include <fstream>
#include <cstdarg>
#include <string>
#include <vector>
#include <map>

bool isquote(char c) {
	return (c == '"') || (c == '\'');
}

bool issymbol(char c) {
	return (c == '*') || (c == ':') || (c == ',') || (c == '[') || (c == ']');
}

std::string format(const char *fmt, ...) {
	std::string result;

	va_list ap;
	va_start(ap, fmt);

	char *tmp = 0;
	vasprintf(&tmp, fmt, ap);
	va_end(ap);

	result = tmp;
	free(tmp);

	return result;
}

struct Lexem {
	enum Type {
		Unknown,
		OneChar,
		Number,
		String,
		Identifier,
		Directive,
		DataType,
		PtrType,
		Operator,
		Reg8,
		Reg32,
		SReg,
		Command
	} type;
	string text;
	int index, begin, end;
	Lexem() : Lexem(Type::Unknown, "", 0, 0, 0) {}
	Lexem(const Type &type, const string &text, const int &index, const int &begin, const int &end) {
		this->text = text;
		this->type = type;
		this->index = index;
		this->begin = begin;
		this->end = end;
	} 
};

string getinfo(Lexem::Type type) {
	switch (type) {
		case Lexem::OneChar: return "one char";
		case Lexem::Number: return "heximal";
		case Lexem::String: return "string";
		case Lexem::Identifier: return "identifier";
		case Lexem::Directive: return "directive";
		case Lexem::DataType: return "data type";
		case Lexem::PtrType: return "ptr type";
		case Lexem::Operator: return "ptr operator";
		case Lexem::Reg8: return "register 8-bit";
		case Lexem::Reg32: return "register 32-bit";
		case Lexem::SReg: return "segment register";
		case Lexem::Command: return "command";
	}
	return "Unknown";
}

map<string, Lexem::Type> keywords = {
	{"ASSUME", Lexem::Directive},
	{"END", Lexem::Directive},
	{"SEGMENT", Lexem::Directive},
	{"ENDS", Lexem::Directive},
	{"EQU", Lexem::Directive},
	{"IF", Lexem::Directive},
	{"ENDIF", Lexem::Directive},

	{"DB", Lexem::DataType},
	{"DW", Lexem::DataType},
	{"DD", Lexem::DataType},

	{"BYTE", Lexem::PtrType},
	//{"WORD", Lexem::PtrType},
	{"DWORD", Lexem::PtrType},
	{"PTR", Lexem::Operator},

	{"AH", Lexem::Reg8},
	{"BH", Lexem::Reg8},
	{"CH", Lexem::Reg8},
	{"DH", Lexem::Reg8},
	{"AL", Lexem::Reg8},
	{"BL", Lexem::Reg8},
	{"CL", Lexem::Reg8},
	{"DL", Lexem::Reg8},

	{"EAX", Lexem::Reg32},
	{"EBX", Lexem::Reg32},
	{"ECX", Lexem::Reg32},
	{"EDX", Lexem::Reg32},
	{"ESI", Lexem::Reg32},
	{"EDI", Lexem::Reg32},
	{"ESP", Lexem::Reg32},
	{"EBP", Lexem::Reg32},

	{"CS", Lexem::SReg},
	{"DS", Lexem::SReg},
	{"SS", Lexem::SReg},
	{"ES", Lexem::SReg},
	{"FS", Lexem::SReg},
	{"GS", Lexem::SReg},

	{"STOSD", Lexem::Command},
	{"DEC", Lexem::Command},
	{"INC", Lexem::Command},
	{"XOR", Lexem::Command},
	{"OR", Lexem::Command},
	{"AND", Lexem::Command},
	{"MOV", Lexem::Command},
	{"ADC", Lexem::Command},
	{"JZ", Lexem::Command}
};

inline bool isonechar(char c) {
	return (c == '+') || (c == '-') || (c == '*') || (c == ':') || (c == ',') || (c == '[') || (c == ']');
};

map<int, string> symbolType = {
	{-3, "ЧИСЛО"},
	{-1, "МІТКА "},
	{ 1, "БАЙТ  "},
	{ 2, "1 СЛОВО"},
	{ 4, "2 СЛОВА"}
};

struct Symbol {
	string segment, value, type, text;

	Symbol() {}
	Symbol(const string &segment, const string &value, const string &type) {
		this->segment = segment;
		this->value = value;
		this->type = type;
		this->text = "";
	}
};

struct Info { 
	int index, count; 
	Info(int index, int count) {
		this->index = index;
		this->count = count;
	}
};

struct Operand {
	enum class Type {
		Undef, Reg, Mem, Imm, Text, Name
	} type;

	vector<Lexem> lexems;
	Info info;

	int ptr, scale, imm, disp;
	Lexem reg, base, index;
	string sreg, name, text;
	bool valid;

	Operand(const Info &info, const vector<Lexem> &lexems) : type(Type::Undef), valid(true), info(info), lexems(lexems) {}

	bool lookup() {
		ptr = scale = imm = disp = 0;
		reg = base = index = Lexem();

		int i = 0, len = lexems.size();

		if ((i < len) && (lexems[i].type == Lexem::Type::PtrType)) {
			string ptr = lexems[i++].text;

			if ((i < len) && (lexems[i].type == Lexem::Type::Operator)) {
				i++;
				if (ptr.compare("byte") == 0) {
					this->ptr = 1;
				} else if (ptr.compare("dword") == 0) {
					this->ptr = 4;
				} else return valid = false;
			} else return valid = false;
			goto is_mem;
		}

		if ((i < len) && ((lexems[i].type == Lexem::Type::Reg8) || (lexems[i].type == Lexem::Type::Reg32))) {
			type = Type::Reg;
			this->reg = lexems[i++];
			return valid = i == len;
		} else if ((i < len) && (lexems[i].type == Lexem::Type::Number)) {
			type = Type::Imm;
			this->imm = stol(lexems[i++].text, 0, 16);
			return valid = i == len;
		} else if ((i < len) && (lexems[i].type == Lexem::Type::String)) {
			type = Type::Text;
			this->text = lexems[i++].text;
			return valid = i == len;
		}

		is_mem:
		if ((i < len) && (lexems[i].type == Lexem::Type::SReg)) {
			const Lexem &sreg = lexems[i++];
			if ((i < len) && (lexems[i].text.compare("[") == 0)) {
				i++;
				this->sreg = sreg.text;
			} else return valid = i == len;
		}

		if ((i < len) && (lexems[i].type == Lexem::Type::Identifier)) {
			type = Type::Name;
			this->name = lexems[i++].text;
			if ((i < len) && (lexems[i].text.compare("[") == 0)) {
				type = Type::Mem;
				i++;
				if ((i < len) && (lexems[i].type == Lexem::Type::Reg32)) {
					this->index = lexems[i++];
					if ((i < len) && (lexems[i].text.compare("*") == 0)) {
						i++;
						if ((i < len) && (lexems[i].type == Lexem::Type::Number)) {
							this->scale = stol(lexems[i++].text, 0, 16);
							if ((i < len) && (lexems[i].text.compare("]"))) i++;
							return i == len;
						} else return valid = false;
					} else return valid = false;
				} else return valid = false;
			}
		} else return valid = false;
	}

	bool isreg() {
		return type == Type::Reg;
	}

	bool ismem() {
		return type == Type::Mem;
	}

	bool isimm() {
		return type == Type::Imm;
	}

	bool istext() {
		return type == Type::Text;
	}

	bool isname() {
		return type == Type::Name;
	}
};

struct Sentence {
	string prefix, bytes, source;
	bool printable, valid, skip;
	unsigned offset, length;

	Info label, name, mnemo;
	vector<Operand> operands;
	vector<Lexem> lexems;

	Sentence(const string &source, const vector<Lexem> lexems) : skip(false), valid(true), source(source), lexems(lexems), label(-1, 0), name(-1, 0), mnemo(-1, 0), printable(false), offset(0) {
		length = 0;
		int len = lexems.size(), i = 0;

		if ((i < len) && (lexems[i].type == Lexem::Identifier)) {
			int index = i++;
			if ((i < len) && (lexems[i].text.compare(":") == 0)) {
				this->label.index = index;
				i++;
			} else {
				this->name.index = index;
			}
		}

		if ((i < len) && ((lexems[i].type == Lexem::Directive) || (lexems[i].type == Lexem::DataType) || (lexems[i].type == Lexem::Command))) {
			this->mnemo.index = i++;
		}

		while (i < len) {
			vector<Lexem> operand_lexems;
			int index = i;
			while (i < len) {
				if (lexems[i].text.compare(",") == 0) {
					i++;
					break;
				}
				operand_lexems.push_back(lexems[i++]);
			}
			Operand operand(Info(index, i - index), operand_lexems);
			valid &= operand.lookup();
			operands.push_back(operand);
		}

		while (operands.size() < 2) {
			operands.push_back(Operand(Info(-1, 0), {}));
		}
	}

	bool lookup(struct Compiler *);
	void printAnalyze(FILE *);
	void printOffset(FILE *);
};

struct IF { bool value; };
struct Compiler {
	map<string, vector<Lexem>> eques;
	map<string, unsigned> segments;
	map<string, Symbol> symbols;
	vector<Sentence> sentences;
	string filename, listing;
	unsigned offset;
	int lineNumber;
	string segment;
	bool error;

	Compiler() : error(false) {}

	vector<Lexem> divide(string &);
	void parse(int argc, char *argv[]);
	void printOffsets();
	void printAnalyze();

	bool SetEqu(const string &text, const vector<Lexem> &lexems) {
		if (eques.find(text) != eques.end()) return false;
		eques[text] = lexems;
		return true;
	}
	bool AddSymbol(const string &text, const Symbol &symbol) {
		if (symbols.find(text) != symbols.end()) return false;
		symbols[text] = symbol;
		return true;
	}

	bool BeginSegment(const string &text) {
		offset = 0;
		if (!segment.empty()) return false;
		segment = text;
		return true;
	}

	bool EndSegment(const string &text, const int &length) {
		if (segment.empty() || (segment.compare(text) != 0)) return false;
		segments[text] = length;
		segment.clear();
		return true;
	}

	vector<IF> ifTable;
};

vector<Lexem> Compiler::divide(string &input) {
	vector<Lexem> lexems;
	for (int i = 0, index = 0; i < input.size();) {
		if (input[i] == ';') break;
		else if (isspace(input[i])) i++;
		else {
			Lexem lexem;
			lexem.index = index++;
			if (isalpha(input[i])) {
				lexem.begin = i;
				while ((i < input.size()) && isalnum(input[i])) {
					lexem.text += toupper(input[i++]);
				}
				lexem.end = i;
				auto keyword = keywords.find(lexem.text);
				lexem.type = (keyword == keywords.end()) ? Lexem::Type::Identifier : keyword->second;
			} else if (isdigit(input[i])) {
				lexem.begin = i;
				while ((i < input.size()) && isalnum(input[i])) {
					lexem.text += toupper(input[i++]);
				}
				lexem.end = i;
				if (toupper(input[i - 1]) == 'H') {
					lexem.type = Lexem::Type::Number;
				} else error = true;
			} else if (isquote(input[i])) {
				char c = input[i++];
				lexem.begin = i;
				while ((i < input.size()) && (input[i] != '\'') && (input[i] != '\n')) {
					lexem.text += input[i++];
				}
				lexem.end = i;
				error = (input[i++] != c);
				lexem.type = Lexem::Type::String;
			} else if (isonechar(input[i])) {
				lexem.type = Lexem::Type::OneChar;
				lexem.begin = i;
				lexem.text = input[i++];
				lexem.end = i;
			} else {
				lexem.begin = i++;
				lexem.end = i;
				error = true;
			}

			if (lexem.type == Lexem::Type::Identifier) {
				const auto &equ = eques.find(lexem.text);
				if (equ != eques.end()) {
					input = input.replace(lexem.begin, lexem.text.size(), symbols[lexem.text].text);
					for (Lexem lexem : equ->second) {
						lexem.index = index++;
						lexems.push_back(lexem);
					}
				} else lexems.push_back(lexem);
			} else lexems.push_back(lexem);
		}
	}
	return lexems;
}

int GetSizeOfImm(int type, int imm) {
	if (type == 1) {
		if ((-256 <= imm) && (imm < 256)) return 1;
		else return -1;
	} else if (type == 2) {
		if ((-65536 <= imm) && (imm < 65536)) {
			return ((-128 <= imm) && (imm < 128)) ? 1 : 2;
		} else return -1;
	}
	return ((-128 <= imm) && (imm < 128)) ? 1 : 4;
}

int GetSizeOfDisp(int disp) {
	return ((0x80 <= disp) && (disp < 0xFF80) || (0x10000 <= disp) && (disp < 0xFFFF80) || (0x1000000 <= disp) && (disp < 0xFFFFFF80)) ? 4 : 1;
}

string GetDefRegSeg(const Lexem &reg) {
	string text = reg.text;
	if ((text.compare("esp") == 0) || (text.compare("ebp") == 0) || (text.compare("sp") == 0) || (text.compare("bp") == 0)) 
		return "ss";
	else if (reg.type == Lexem::Type::Reg32) return "ds";
	else return "";
}

bool Sentence::lookup(Compiler *view) {
	if (view->error) return valid = false;

	int len = lexems.size();

	if (label.index != -1) {
		if (!view->AddSymbol(lexems[label.index].text, Symbol(view->segment, format(" %.4X ", view->offset), "L NEAR"))) {
			return valid = false;
		}
		printable = true;
	} else if (mnemo.index != -1) {
		auto &mnemocode = lexems[mnemo.index];
		if (name.index != -1) {
			if (mnemocode.text.compare("SEGMENT") == 0) {
				if (!view->BeginSegment(lexems[name.index].text)) return valid = false;
				printable = true;
			} else if (mnemocode.text.compare("ENDS") == 0) {
				if (!view->EndSegment(lexems[name.index].text, view->offset)) return valid = false;
				printable = true;
			} else if (mnemocode.text.compare("EQU") == 0) {
				vector<Lexem> equ;
				int i = mnemo.index + 1;
				if (i < len) {
					while (i < len) {
						equ.push_back(lexems[i++]);
					}
				} else return false;
				int count = equ.size();

				Symbol symbol;
				symbol.text = source.substr(equ[0].begin, equ[count - 1].end);
				if ((count == 1) && (equ[0].type == Lexem::Number)) {
					symbol.type = "NUMBER";
					symbol.value = format("%.4X", stol(symbol.text, 0, 16));
					prefix = format(" = %s ", symbol.value.c_str());
				} else if (count > 0) {
					symbol.type = "TEXT";
					symbol.value = symbol.text;
					prefix = " =     ";
				} else return valid = false;
				if (!view->AddSymbol(lexems[name.index].text, symbol)) return valid = false;
				if (!view->SetEqu(lexems[name.index].text, equ)) return valid = false;
			} else if (mnemocode.type == Lexem::DataType) {
				Symbol symbol;
				symbol.value = format(" %.4X ", view->offset);
				symbol.segment = view->segment;

				if (mnemocode.text.compare("DB") == 0) {
					symbol.type = "L BYTE";
					if (operands[0].valid) {
						if (operands[0].istext()) {
							length = operands[0].text.size();
						} else if (operands[0].isimm()) {
							length = 1;
						} else return valid = false;
					} else return valid = false;
				} else if (mnemocode.text.compare("DW") == 0) {
					symbol.type = "L WORD";
					if (operands[0].valid) {
						if (operands[0].isimm()) {
							length = 2;
						} else return valid = false;
					} else return valid = false;
				} else if (mnemocode.text.compare("DD") == 0) {
					symbol.type = "L DWORD";
					if (operands[0].valid) {
						if (operands[0].isimm()) {
							length = 4;
						} else return valid = false;
					} else return valid = false;
				} 

				if (!view->AddSymbol(lexems[name.index].text, symbol)) {
					return valid = false;
				}
				printable = true;
			}
		} else if (mnemocode.text.compare("IF") == 0) {
			if (operands[0].isimm()) {
				IF context;
				context.value = operands[0].imm;
				skip = !context.value;
				view->ifTable.push_back(context);
			} else return valid = false;
		} else if (mnemocode.text.compare("ENDIF") == 0) {
			if (view->ifTable.empty()) return valid = false;
			skip = !view->ifTable.back().value;
			view->ifTable.pop_back();
		} else if (!view->ifTable.empty() && !view->ifTable.back().value) {
			skip = true;
		} else if (mnemocode.text.compare("END") == 0) {

		} else if (mnemocode.type == Lexem::Command) {
			printable = true;

			if (mnemocode.text.compare("STOSD") == 0) {
				length = 1;
			} else if (mnemocode.text.compare("DEC") == 0) {
				if (!operands[0].isreg()) return valid = false;
				length = 1;
			} else if (mnemocode.text.compare("INC") == 0) {
				if (!operands[0].ismem()) return valid = false;
				Lexem &index = operands[1].index;
				string &sreg = operands[1].sreg;

				length = 1/*instr*/ + 1/*mod*/ + 1/*sib*/ + 4/*ident*/;

				if (!sreg.empty() && !sreg.compare(GetDefRegSeg(index)) || GetDefRegSeg(index).compare("ds") || view->segment.compare(view->symbols[operands[1].text].segment)) {
					//length += 1;
				}
			} else if (mnemocode.text.compare("XOR") == 0) {
				if (operands[0].isreg() && operands[1].isreg()) {
					if (operands[0].reg.type == operands[1].reg.type) {
						length = 2;
					} else return valid = false;
				} else return valid = false;
			} else if (mnemocode.text.compare("OR") == 0) {
				if (operands[0].isreg() && operands[1].ismem()) {
					const Lexem &reg = operands[0].reg;
					Lexem &index = operands[1].index;
					string &sreg = operands[1].sreg;

					length = 1/*instr*/ + 1/*mod*/ + 1/*sib*/ + 4/*ident*/;

					if (!sreg.empty() && !sreg.compare(GetDefRegSeg(index)) || GetDefRegSeg(index).compare("ds") || view->segment.compare(view->symbols[operands[1].text].segment)) {
						// length += 1;
					}
				} else return valid = false;
			} else if (mnemocode.text.compare("AND") == 0) {
				if (operands[0].isimm() && operands[1].isreg()) {
					const Lexem &reg = operands[0].reg;
					const string &sreg = operands[1].sreg;
					Lexem &index = operands[1].index;

					length = 1/*instr*/ + 1/*mod*/ + 1/*sib*/ + 4/*ident*/;

					if (!sreg.empty() && !sreg.compare(GetDefRegSeg(index)) || GetDefRegSeg(index).compare("ds") || view->segment.compare(view->symbols[operands[1].text].segment)) {
						// length += 1;
					}
				} else return valid = false;
			} else if (mnemocode.text.compare("MOV") == 0) {
				if (operands[0].isreg() && operands[1].isimm()) {
					length = 1 + GetSizeOfImm(operands[0].lexems[0].type == Lexem::Type::Reg8 ? 1 : 4, operands[1].imm & 0xFFFFFFFF); 
				} else return valid = false;
			} else if (mnemocode.text.compare("ADC") == 0) {
				if (operands[0].ismem() && operands[1].isimm()) {
					length = 1/*instr*/ + 1/*mod*/ + 1/*sib*/ + 4/*ident*/ + GetSizeOfImm(operands[0].ptr, operands[1].imm & 0xFFFFFFFF); 
				} else return valid = false;
			} else if (mnemocode.text.compare("JZ") == 0) {
				if (operands[0].valid) {
					if (operands[0].ismem()) {
						Lexem &index = operands[1].index;
						string &sreg = operands[1].sreg;

						length = 1/*instr*/ + 1/*mod*/ + 1/*sib*/ + 4/*ident*/;

						if (!sreg.empty() && !sreg.compare(GetDefRegSeg(index)) || GetDefRegSeg(index).compare("ds") || view->segment.compare(view->symbols[operands[1].text].segment)) {
							// length += 1;
						}
					} else if (operands[0].isname()) {
						string &sreg = operands[1].sreg;

						length = view->symbols.find(operands[1].name) != view->symbols.end() ? 2 : 6;

						// if (!sreg.empty() && sreg.compare("ds")) length += 1;
					} else return valid = false;
				} return valid = false;
			}		
		}
	} else if ((name.index != -1) || !operands.empty()) {
		return valid = false;
	}
	return true;
}

void Sentence::printAnalyze(FILE *file) {
	if (source.empty()) return;
	fprintf(file, " Label  Mnemocode  1st operand  2nd operand\n");
	fprintf(file, " index    index    index count  index count\n");

	fprintf(file, " %5i  %9i  %5i %5i  %5i %5i\n\n", label.index & name.index, mnemo.index, operands[0].info.index, operands[0].info.count, operands[1].info.index, operands[1].info.count);	
	int index = 0;
	for (auto &lexem : lexems) {
		fprintf(file, "%-2d | %11s | %2i | %16s |\n", index++, lexem.text.c_str(), lexem.text.size(), getinfo(lexem.type).c_str());
	}
	fprintf(file, "\n");
}

void Sentence::printOffset(FILE *file) {
	if (skip) return;

	if (printable) {
		fprintf(file, " %.4X ", offset);
	} else if (!prefix.empty()) {
		fprintf(file, prefix.c_str());
	} else fprintf(file, "    ");

	fprintf(file, "\t\t%s\n", source.c_str());
}

void Compiler::parse(int argc, char *argv[]) {
	if (argc > 1) filename = argv[1];
	auto pos = filename.find_last_of(".");
	cout << "Source filename[.asm]: " << filename;
	if (argc <= 1) while (!getline(cin, filename)); else cout << endl;
	if (filename.find_last_of(".") == string::npos) filename += ".asm";

	if (argc > 2) listing = argv[2];
	cout << "Source listing[.lst]: " << listing;
	if (argc <= 2) getline(cin, listing); else cout << endl;
	if (listing.find_last_of(".") == string::npos) listing += ".lst";
	cout << endl;

	ifTable.clear();

	lineNumber = 0;
	string line;
	ifstream file(filename);
	offset = 0;
	while (getline(file, line)) {
		lineNumber ++;
		const auto &lexems = divide(line);
		Sentence sentence(line, lexems);
		sentence.lookup(this);
		sentence.offset = offset;
		offset += sentence.length;
		sentences.push_back(sentence);
	}
	file.close();
}

void Compiler::printAnalyze() {
	FILE *file = fopen((filename.substr(0, filename.find_last_of(".")) + ".lex").c_str(), "w");
	int lineNumber = 0;
	for (auto &sentence : sentences) {
		fprintf(file, " %s\n", sentence.source.c_str());
		if (sentence.valid) {
			sentence.printAnalyze(file);
		} else {
			fprintf(file, "%s(%d): error\n", filename.c_str(), lineNumber);
		}
		lineNumber++;
	}
	fclose(file);
}

void Compiler::printOffsets() {
	FILE *file = fopen(listing.c_str(), "w");
	int lineNumber = 0;
	for (auto &sentence : sentences) {
		sentence.printOffset(file);
		// if (!sentence.valid) fprintf(file, "%s(%d): error\n", filename.c_str(), lineNumber);
		lineNumber++;
	}

	fprintf(file, "\n\n                N a m e         	Size	Length\n\n");
	for (auto &segment : segments) {
		fprintf(file, "%-32s\t%-7s\t%-.4X\n", segment.first.c_str(), "32 Bit", segment.second);
	}
	
	fprintf(file, "\nSymbols:\n                N a m e         	Type	 Value	 Attr\n");
	for (auto symbol : symbols) {
		fprintf(file, "%-32s\t%-7s\t%-s\t%s\n", symbol.first.c_str(), symbol.second.type.c_str(), symbol.second.value.c_str(), symbol.second.segment.c_str());
	}
	fprintf(file, "\n");
	fclose(file);
}

int main(int argc, char *argv[]) {
	argc += 2;
	argv[1] = &(string("test.asm")[0]);
	argv[2] = &(string("test.lst")[0]);

	Compiler *compiler = new Compiler;
	compiler->parse(argc, argv);
	compiler->printAnalyze();
	compiler->printOffsets();
}