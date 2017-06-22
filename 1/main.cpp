using namespace std;

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>

enum LexemType{
	UNKNOWN,
	ONE_CHAR,
	USER_IDENT,
	STR_CONST,
	BIN_CONST,
	DEC_CONST,
	HEX_CONST,
	DIRECTIVE,
	DATA_TYPE,
	PTR_TYPE,
	PTR_PTR,
	SEG_REG,
	REG_32,
	REG_8,
	COMMAND
};

struct Lexem {
	LexemType type;
	string text;
	int index;
};

map<LexemType, string> lexemInfo = {
	{UNKNOWN, "unknown"},
	{ONE_CHAR, "one char"},
	{USER_IDENT, "identifier"},
	{STR_CONST, "string"},
	{BIN_CONST, "binary"},
	{DEC_CONST, "decimal"},
	{HEX_CONST, "hexadecimal"},
	{DIRECTIVE, "directive"},
	{DATA_TYPE, "data type"},
	{PTR_TYPE, "ptr type"},
	{PTR_PTR, "operator"},
	{SEG_REG, "segment register"},
	{REG_32, "32-bit register"},
	{REG_8, "8-bit register"},
	{COMMAND, "command"}
};

map<string, LexemType> lexemType = {
	{"assume", DIRECTIVE},
	{"end", DIRECTIVE},
	{"segment", DIRECTIVE},
	{"ends", DIRECTIVE},
	{"equ", DIRECTIVE},
	{"if", DIRECTIVE}, 
	{"else", DIRECTIVE},
	{"endif", DIRECTIVE},

	{"db", DATA_TYPE},
	{"dw", DATA_TYPE},
	{"dd", DATA_TYPE},

	{"byte", PTR_TYPE},
	{"dword", PTR_TYPE},
	{"ptr", PTR_PTR},

	{"eax", REG_32},
	{"ebx", REG_32},
	{"ecx", REG_32},
	{"edx", REG_32},
	{"esi", REG_32},
	{"edi", REG_32},
	{"esp", REG_32},
	{"ebp", REG_32},

	{"ah", REG_8},
	{"bh", REG_8},
	{"ch", REG_8},
	{"dh", REG_8},
	{"al", REG_8},
	{"bl", REG_8},
	{"cl", REG_8},
	{"dl", REG_8},

	{"cs", SEG_REG},
	{"ds", SEG_REG},
	{"ss", SEG_REG},
	{"es", SEG_REG},
	{"fs", SEG_REG},
	{"gs", SEG_REG},

	{"pusha", COMMAND},
	{"inc", COMMAND},
	{"dec", COMMAND},
	{"xchg", COMMAND},
	{"lea", COMMAND},
	{"and", COMMAND},
	{"mov", COMMAND},
	{"or", COMMAND},
	{"jb", COMMAND}
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
	unsigned value;
	string segment;
	int type;
};

struct Matcher {
	vector<Lexem> lexems;
	int index;

	Matcher() : index(0) {}
	Matcher(const vector<Lexem> &lexems) : lexems(lexems), index(0) {}

	void reset() { index = 0; }
	void next() { if (index < lexems.size()) index++; }
	Lexem &peek() {
		static Lexem lexem;
		return index < lexems.size() ? lexems[index] : lexem;
	}
	Lexem &get() {
		Lexem &lexem = peek();
		next();
		return lexem;
	}
	void push(const Lexem &lexem) {
		lexems.push_back(lexem);
	}

	bool compare(const string &text) {
		return peek().text.compare(text) == 0;
	}

	bool confirm(const string &text) {
		return compare(text) ? next(), true : false;
	}

	bool compare(const LexemType &type) {
		return peek().type == type;
	}

	bool confirm(const LexemType &type) {
		return compare(type) ? next(), true : false;
	}

	bool has() {
		return index < lexems.size();
	}
};

struct Operand {
	vector<Lexem> lexems;

	int ptr, scale, imm, disp;
	Lexem reg, base, index;
	string sreg, ident, text;

	Operand(const vector<Lexem> &lexems) : lexems(lexems) { 
		Matcher matcher(lexems);

		ptr = 0;
		scale = 0;
		imm = 0;
		disp = 0;
		reg = {LexemType::UNKNOWN, "", 0};
		base = {LexemType::UNKNOWN, "", 0};
		index = {LexemType::UNKNOWN, "", 0};

		if (matcher.compare(PTR_TYPE)) {
			string ptr = matcher.get().text;

			if (matcher.confirm(PTR_PTR)) {
				if (ptr.compare("byte") == 0) {
					this->ptr = 1;
				} else if (ptr.compare("word") == 0) {
					this->ptr = 2;
				} else if (ptr.compare("dword") == 0) {
					this->ptr = 4;
				}
			}
		}

		if (matcher.compare(REG_8) || matcher.compare(REG_32)) {
			this->reg = matcher.get();
		} else if (matcher.compare(BIN_CONST)) {
			this->imm = stol(matcher.get().text, 0, 2);
		} else if (matcher.compare(DEC_CONST)) {
			this->imm = stol(matcher.get().text, 0, 10);
		} else if (matcher.compare(HEX_CONST)) {
			this->imm = stol(matcher.get().text, 0, 16);
		} else if (matcher.compare(STR_CONST)) {
			this->text = matcher.get().text;
		}

		if (matcher.compare(SEG_REG)) {
			Lexem &sreg = matcher.get();
			if (matcher.confirm(":")) {
				this->sreg = sreg.text;
			}
		}

		if (matcher.compare(USER_IDENT)) {
			this->ident = matcher.get().text;
		}

		if (matcher.confirm("[")) {
			if (matcher.compare(REG_8)) {
				this->base = matcher.get();
				if (matcher.confirm("+")) {
					this->index = matcher.get();
					this->scale = 1;
					if (matcher.confirm("+")) {
						if (matcher.compare(BIN_CONST)) {
							this->disp = stol(matcher.get().text, 0, 2);
						} else if (matcher.compare(DEC_CONST)) {
							this->disp = stol(matcher.get().text, 0, 10);
						} else if (matcher.compare(HEX_CONST)) {
							this->disp = stol(matcher.get().text, 0, 16);
						}
					}
				}
			}
			matcher.confirm("]");
		}
	}
};

struct Sentence {
	vector<Operand> operands;
	vector<Lexem> mnemocode;
	vector<Lexem> lexems;
	vector<Lexem> label;
	string input, bytes;
	unsigned length;
	unsigned offset;

	Sentence(const string &input, const vector<Lexem> &lexems) : input(input), lexems(lexems), length(0), offset(0) {
		Matcher matcher(lexems);

		if (matcher.compare(USER_IDENT)) {
			label.push_back(matcher.get());
			if (matcher.compare(":")) 
				label.push_back(matcher.get());
		}

		if (matcher.compare(DIRECTIVE) || matcher.compare(COMMAND) || matcher.compare(DATA_TYPE)) {
			mnemocode.push_back(matcher.get());
		}

		while (matcher.has()) {
			vector<Lexem> lexems;
			while (matcher.has() && !matcher.confirm(",")) {
				lexems.push_back(matcher.get());
			}
			operands.push_back(lexems);
		}
	} 
};

map<string, string> segmentPrefixes = {
	{"cs", "2E"},
	{"ds", "3E"},
	{"ss", "36"},
	{"es", "26"},
	{"fs", "64"},
	{"gs", "65"}
};

struct IF { bool value; };
struct FirstView {
	map<string, unsigned> segment_table;
	map<string, Symbol> symbol_table;
	map<string, string> assume_table;
	vector<Sentence> sentences;
	string default_segment;
	Sentence *sentence;
	string filename;
	unsigned offset;
	string segment;
	FILE *outfile;
	bool printed;

	vector<Lexem> divide(const string &);
	void run(const string &);
	void printOffset();
	void printLexical();

	vector<IF> ifTable;
};

vector<Lexem> FirstView::divide(const string &input) {
	vector<Lexem> lexems;
	for (int i = 0, index = 0; i < input.size();) {
		if (input[i] == ';') break;
		else if (isspace(input[i])) i++;
		else {
			Lexem lexem;
			lexem.index = index++;
			if (isalpha(input[i])) {
				while ((i < input.size()) && isalnum(input[i])) {
					lexem.text += tolower(input[i++]);
				}
				auto type = lexemType.find(lexem.text);
				if (type != lexemType.end()) {
					lexem.type = type->second;
				} else lexem.type = USER_IDENT;
			} else if (isdigit(input[i])) {
				while ((i < input.size()) && isxdigit(input[i])) {
					lexem.text += tolower(input[i++]);
				}
				if (tolower(input.back()) == 'b') {
					lexem.type = BIN_CONST;
					lexem.text.pop_back();
				} else if (tolower(input[i]) == 'h') {
					lexem.type = HEX_CONST;
					i++;
				} else {
					if (tolower(input[i]) == 'd') i++;
					lexem.type = DEC_CONST;
				}
			} else if (input[i] == '\'') {
				i++;
				while ((i < input.size()) && (input[i] != '\'')) {
					lexem.text += input[i++];
				}
				i++;
				lexem.type = STR_CONST;
			} else if (isonechar(input[i])) {
				lexem.text = input[i++];
				lexem.type = ONE_CHAR;
			} else i++;

			if (lexem.type == USER_IDENT) {
				map<string, Symbol>::iterator equ = symbol_table.find(lexem.text);
				if ((equ != symbol_table.end()) && (equ->second.type == -3)) {
					lexem.type = DEC_CONST;
					lexem.text = to_string(equ->second.value);
				}
			}

			lexems.push_back(lexem);
		}
	}
	return lexems;
}

void FirstView::printOffset() {
	if (printed) return;
	printed = true;
	fprintf(outfile, " %.4X ", offset);
	if (!sentence->bytes.empty()) {
		fprintf(outfile, " %s ", sentence->bytes.c_str());
	}
}

int GetSizeOfIMM(int type, int imm) {
	if (type == 1) {
		if ((-256 <= imm) && (imm < 256)) return 1;
		else {
			// print("Value out of range at line " + text(self.current_line));
			exit(1);
		}
	} else if (type == 2) {
		if ((-65536 <= imm) && (imm < 65536)) {
			return ((-128 <= imm) && (imm < 128)) ? 1 : 2;
		} else {
			// print("Value out of range at line " + text(self.current_line));
			exit(1);
		}
	}
	return ((-128 <= imm) && (imm < 128)) ? 1 : 4;
}

string GetDefRegSeg(const Lexem &reg) {
	string text = reg.text;
	if ((text.compare("esp") == 0) || (text.compare("ebp") == 0) || (text.compare("sp") == 0) || (text.compare("bp") == 0)) 
		return "ss";
	else if (reg.type == LexemType::REG_32) return "ds";
	return "";
}

void FirstView::run(const string &filepath) {
	size_t index = filepath.find_last_of(".");
	if (index == -1) filename = filepath;
	else filename = filepath.substr(0, index);

	outfile = fopen((filename + ".lst").c_str(), "w");
	ifTable.clear();

	ifstream file(filename + ".asm");
	string line;

	int current_line = -1;
	while (getline(file, line)) {
		current_line ++;
		Sentence sentence(line, divide(line));
		this->sentence = &sentence;
		printed = false;

		if (sentence.label.size() == 2) {
			Symbol symbol;
			symbol.type = -1;
			symbol.value = offset;
			symbol.segment = segment;
			symbol_table[sentence.label[0].text] = symbol;
			printOffset();
		}

		if (!sentence.mnemocode.empty()) {
			if (sentence.mnemocode[0].text.compare("if") == 0) {
				IF context;
				context.value = sentence.operands[0].imm;
				ifTable.push_back(context);
				if (!context.value) continue;
			} else if (sentence.mnemocode[0].text.compare("else") == 0) {
				IF &context = ifTable.back();
				context.value = !context.value;
				if (!context.value) continue;
			} else if (sentence.mnemocode[0].text.compare("endif") == 0) { 
				ifTable.pop_back();
			} else if (!ifTable.empty() && !ifTable.back().value) {
				continue;
			} else if (sentence.mnemocode[0].text.compare("equ") == 0) {
				fprintf(outfile, " = ");
				Symbol symbol;
				symbol.type = -3;
				symbol.segment = "";
				// if (sentence.operands[0].table[9].first.isNumber()) {
					symbol.value = sentence.operands[0].imm;	
					fprintf(outfile, "%.4X", symbol.value);
				// } else symbol.value = 0;
				symbol_table[sentence.label[0].text] = symbol;
			} else if (sentence.mnemocode[0].text.compare("assume") == 0) {
				for (Operand &operand : sentence.operands) {
					assume_table[operand.sreg] = operand.ident;
				}
			} else if (sentence.mnemocode[0].text.compare("segment") == 0) {
				offset = 0;
				printOffset();
				if (sentence.label.size() == 1)
					segment = sentence.label[0].text;
			} else if (sentence.mnemocode[0].text.compare("ends") == 0) {
				printOffset();
				segment_table[segment] = offset;
				offset = 0;
				segment.clear();
			} else if (sentence.mnemocode[0].text.compare("end") == 0) {
			} else if (sentence.mnemocode[0].type == DATA_TYPE) {
				printOffset();
				Symbol symbol;
				symbol.value = offset;
				symbol.segment = segment;

				if (sentence.mnemocode[0].text.compare("db") == 0) {
					symbol.type = 1;
					if (!sentence.operands[0].text.empty()) {
						sentence.length = sentence.operands[0].text.size();
					} else sentence.length = 1; 
				} else if (sentence.mnemocode[0].text.compare("dw") == 0) {
					symbol.type = 2;
					sentence.length = 2;
				} else if (sentence.mnemocode[0].text.compare("dd") == 0) {
					symbol.type = 4;
					sentence.length = 4;
				} 

				if (sentence.label.size() == 1)
					symbol_table[sentence.label[0].text] = symbol;
			} else {
				printOffset();
				if (sentence.mnemocode[0].text.compare("pusha") == 0) {
					sentence.length = 2;
				} else if (sentence.mnemocode[0].text.compare("inc") == 0) {
					if (sentence.operands.size() > 0) {
						LexemType &reg = sentence.operands[0].reg.type;
						if (reg == LexemType::REG_8) sentence.length = 2;
						else if (reg == LexemType::REG_32) sentence.length = 1;
					}
				} else if (sentence.mnemocode[0].text.compare("dec") == 0) {
					if (sentence.operands.size() > 0) {
						string sreg = sentence.operands[0].sreg;
						Lexem &base = sentence.operands[0].base;
						Lexem &index = sentence.operands[0].index;
						string ident = sentence.operands[0].ident;
						int disp = sentence.operands[0].disp;

						sentence.length = 3;// + GetSizeOfIMM(sentence.operands[0].ptr, sentence.operands[0].disp & 0xFFFFFFFF);

						if ((0x80 <= disp) && (disp < 0xFF80)) {
							sentence.length += 4;
						} else if ((0x10000 <= disp) && (disp < 0xFFFF80)) {
							sentence.length += 4;
						} else if ((0x1000000 <= disp) && (disp < 0xFFFFFF80)) {
							sentence.length += 4;
						} else sentence.length += 1;
						
						if (!sreg.empty() && (sentence.operands[0].sreg.compare(GetDefRegSeg(base)) != 0)) {
							sentence.length += 1;
						} else if ((base.type != LexemType::UNKNOWN) && (GetDefRegSeg(base).compare("ds") != 0)) {
							sentence.length += 1;
						} else {
							Symbol symbol = symbol_table[ident];
							for (auto &assume : assume_table) {
								if (assume.second.compare(symbol.segment) == 0) {
									if (assume.first.compare("ds") != 0) {
										sentence.length += 1;
									}
								}
							}
						}

						if ((base.text.compare("esi") == 0) || (base.text.compare("edi") == 0) || (index.text.compare("esi") == 0) || (index.text.compare("edi") == 0)) {
							sentence.length += 1;
						}
					}
				} else if (sentence.mnemocode[0].text.compare("xchg") == 0) {
					if (sentence.operands.size() > 1) {
						if ((sentence.operands[0].reg.type == LexemType::REG_32) && (sentence.operands[1].reg.type == LexemType::REG_32)) {
							sentence.length = 1;
							if (sentence.operands[1].reg.text.compare("eax") != 0) sentence.length += 1;
						}						
					}
				} else if (sentence.mnemocode[0].text.compare("lea") == 0) {
					if (sentence.operands.size() > 1) {
						string sreg = sentence.operands[0].sreg;
						Lexem &base = sentence.operands[0].base;
						Lexem &index = sentence.operands[0].index;
						string ident = sentence.operands[0].ident;
						int disp = sentence.operands[0].disp;

						sentence.length = 3;// + GetSizeOfIMM(sentence.operands[0].ptr, sentence.operands[0].disp & 0xFFFFFFFF);

						if ((0x80 <= disp) && (disp < 0xFF80)) {
							sentence.length += 4;
						} else if ((0x10000 <= disp) && (disp < 0xFFFF80)) {
							sentence.length += 4;
						} else if ((0x1000000 <= disp) && (disp < 0xFFFFFF80)) {
							sentence.length += 4;
						} else sentence.length += 1;

						if (!sreg.empty() && (sentence.operands[0].sreg.compare(GetDefRegSeg(base)) != 0)) {
							sentence.length += 1;
						} else if ((base.type != LexemType::UNKNOWN) && (GetDefRegSeg(base).compare("ds") != 0)) {
							sentence.length += 1;
						} else {
							Symbol symbol = symbol_table[ident];
							for (auto &assume : assume_table) {
								if (assume.second.compare(symbol.segment) == 0) {
									if (assume.first.compare("ds") != 0) {
										sentence.length += 1;
									}
								}
							}
						}
					}
				} else if (sentence.mnemocode[0].text.compare("and") == 0) {
					if (sentence.operands.size() > 1) {
						string sreg = sentence.operands[0].sreg;
						Lexem &base = sentence.operands[0].base;
						Lexem &index = sentence.operands[0].index;
						string ident = sentence.operands[0].ident;
						int disp = sentence.operands[0].disp;

						sentence.length = 3;// + GetSizeOfIMM(sentence.operands[0].ptr, sentence.operands[0].disp & 0xFFFFFFFF);

						if ((0x80 <= disp) && (disp < 0xFF80)) {
							sentence.length += 4;
						} else if ((0x10000 <= disp) && (disp < 0xFFFF80)) {
							sentence.length += 4;
						} else if ((0x1000000 <= disp) && (disp < 0xFFFFFF80)) {
							sentence.length += 4;
						} else sentence.length += 1;

						if (!sreg.empty() && (sentence.operands[0].sreg.compare(GetDefRegSeg(base)) != 0)) {
							sentence.length += 1;
						} else if ((base.type != LexemType::UNKNOWN) && (GetDefRegSeg(base).compare("ds") != 0)) {
							sentence.length += 1;
						} else {
							Symbol symbol = symbol_table[ident];
							for (auto &assume : assume_table) {
								if (assume.second.compare(symbol.segment) == 0) {
									if (assume.first.compare("ds") != 0) {
										sentence.length += 1;
									}
								}
							}
						}

						if ((base.text.compare("esi") == 0) || (base.text.compare("edi") == 0) || (index.text.compare("esi") == 0) || (index.text.compare("edi") == 0)) {
							sentence.length += 1;
						}
					}
				} else if (sentence.mnemocode[0].text.compare("mov") == 0) {
					if (sentence.operands.size() > 1) {
						int reg = (sentence.operands[0].reg.type == LexemType::REG_8) ? 1 : ((sentence.operands[0].reg.type == LexemType::REG_32) ? 4 : 0);
						sentence.length = 1 + reg;
					}
				} else if (sentence.mnemocode[0].text.compare("or") == 0) {
					if (sentence.operands.size() > 1) {
						string sreg = sentence.operands[0].sreg;
						Lexem &base = sentence.operands[0].base;
						Lexem &index = sentence.operands[0].index;
						string ident = sentence.operands[0].ident;
						int disp = sentence.operands[0].disp;

						sentence.length = 3 + GetSizeOfIMM(sentence.operands[0].ptr, sentence.operands[0].imm & 0xFFFFFFFF);

						if ((0x80 <= disp) && (disp < 0xFF80)) {
							sentence.length += 4;
						} else if ((0x10000 <= disp) && (disp < 0xFFFF80)) {
							sentence.length += 4;
						} else if ((0x1000000 <= disp) && (disp < 0xFFFFFF80)) {
							sentence.length += 4;
						} else sentence.length += 1;

						if (!sreg.empty() && (sentence.operands[0].sreg.compare(GetDefRegSeg(base)) != 0)) {
							sentence.length += 1;
						} else if ((base.type != LexemType::UNKNOWN) && (GetDefRegSeg(base).compare("ds") != 0)) {
							sentence.length += 1;
						} else {
							Symbol symbol = symbol_table[ident];
							for (auto &assume : assume_table) {
								if (assume.second.compare(symbol.segment) == 0) {
									if (assume.first.compare("ds") != 0) {
										sentence.length += 1;
									}
								}
							}
						}

						if ((base.text.compare("esi") == 0) || (base.text.compare("edi") == 0) || (index.text.compare("esi") == 0) || (index.text.compare("edi") == 0)) {
							sentence.length += 1;
						}
					}
				} else if (sentence.mnemocode[0].text.compare("jb") == 0) {
					if (sentence.operands.size() > 0) {
						string ident = sentence.operands[0].ident;

						if (ident.empty()) {
							printf("Illegal operand type at line %d", current_line);
							exit(1);
						}
						sentence.length = 2;

						if (symbol_table.find(ident) == symbol_table.end()) {
							sentence.length += 4;
						}
					}
				}
			}
		}
		sentence.offset = offset;
		offset += sentence.length;
		sentences.push_back(sentence);	
		fprintf(outfile, "\t\t%s\n", sentence.input.c_str());

	}
	file.close();

	fprintf(outfile, "\n\n                І м ' я         	Розмір	Довжина\n\n");
	for (auto &segment : segment_table) {
		fprintf(outfile, "%-32s\t%-7s\t%-.4X\n", segment.first.c_str(), "32 Bit", segment.second);
	}
	
	fprintf(outfile, "\nСимволи:\n                І м ' я         	Тип 	Дані	Атрибути\n\n");
	for (auto &symbol : symbol_table) {
		fprintf(outfile, "%-32s\t%-7s\t%-.4X\t%s\n", symbol.first.c_str(), symbolType[symbol.second.type].c_str(), symbol.second.value, symbol.second.segment.c_str());
	}

	fprintf(outfile, "\n%-32s\tTEXT  %s\n", "@FILENAME", filename.c_str());
	fclose(outfile);
}

void FirstView::printLexical() {
	FILE *file = fopen((filename + "_lexical.txt").c_str(), "w");
	for (Sentence &sentence : sentences) {
		if (sentence.input.empty()) continue;
		
		fprintf(file, "-> %s\n", sentence.input.c_str());
		fprintf(file, " Мітка   Мнемокод    1-ий операнд      2-ий операнд\n");
		fprintf(file, " індекс   індекс   індекс кількість  індекс кількість\n");

		int label = sentence.label.empty() ? -1 : (sentence.label[0].index + 1);
		int mnemo = sentence.mnemocode.empty() ? -1 : (sentence.mnemocode[0].index + 1);
		int o1index = sentence.operands.size() > 0 ? (sentence.operands[0].lexems[0].index + 1) : -1;
		int o1count = sentence.operands.size() > 0 ? sentence.operands[0].lexems.size() : 0;
		int o2index = sentence.operands.size() > 1 ? (sentence.operands[1].lexems[0].index + 1) : -1;
		int o2count = sentence.operands.size() > 1 ? sentence.operands[1].lexems.size(): 0;
		fprintf(file, " %6i  %8i  %6i %9i  %6i %9i\n\n", label, mnemo, o1index, o1count, o2index, o2count);	
		int index = 0;
		for (Lexem &lexem : sentence.lexems) {
			fprintf(file, "%-2d | %11s | %2i | %16s |\n", ++index, lexem.text.c_str(), lexem.text.size(), lexemInfo[lexem.type].c_str());
		}
		fprintf(file, "\n");
	}
	fclose(file);
}

int main() {
	FirstView *firstView = new FirstView;
	firstView->run("test.asm");
	firstView->printLexical();
}