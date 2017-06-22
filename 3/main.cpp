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

	{"aaa", COMMAND},
	{"inc", COMMAND},
	{"div", COMMAND},
	{"add", COMMAND},
	{"cmp", COMMAND},
	{"and", COMMAND},
	{"imul", COMMAND},
	{"or", COMMAND},
	{"jbe", COMMAND}
};

inline bool isonechar(char c) {
	return (c == '+') || (c == '-') || (c == '*') || (c == ':') || (c == ',') || (c == '[') || (c == ']');
};

struct Value {
	string text;
	Lexem lexem;
	long number;
	int type;

	Value() : type(-1) {}
	Value(const long &value) : number(value), type(0) {}
	Value(const Lexem &value) : lexem(value), type(1) {}
	Value(const string &value) : text(value), type(2) {}

	bool isNumber() { return type == 0; }
	bool isToken() { return type == 1; }
	bool isString() { return type == 2; }

	string toString() {
		switch (type) {
			case 0: return to_string(number);
			case 1: return lexem.text;
			case 2: return text;
		}
	}
};

struct Variable {
	unsigned value;
	string segment;
	string type;
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

	struct Element {
		bool present;
		Value first, second;
		Element() : present(false), first(-1), second(-1) {}
		void set(bool present, const Value &first, const Value &second) {
			this->present = present;
			this->first = first;
			this->second = second;
		}
	} table[10];

	Operand(const vector<Lexem> &lexems) : lexems(lexems) { 
		for (int i = 0; i < 10; i++) {
			table[0].set(false, -1, -1);
		}
	}

	bool isREG() {
		for (int i = 1; i < 10; i++) {
			if (table[i].present) return false;
		}
		return table[0].present;
	}

	bool isIMM() {
		for (int i = 2; i < 9; i++) {
			if (table[i].present) return false;
		}
		return !table[0].present && table[9].present;
	}

	bool isMEM() {
		if (table[0].present || table[9].present) return false;
		for (int i = 3; i < 9; i++) {
			if (table[i].present) return true;
		}
		return false;
	}

	bool isLabel() {
		if (table[0].present || table[1].present) return false;
		for (int i = 5; i < 10; i++) {
			if (table[i].present) return false;
		}
		return table[3].present || table[4].present;
	}
};

struct Sentence {
	vector<unsigned char> bytes;
	vector<Operand> operands;
	vector<Lexem> mnemocode;
	vector<Lexem> lexems;
	vector<Lexem> label;
	unsigned length;
	unsigned offset;
	string input;

	Sentence(const string &input, const vector<Lexem> &lexems) : input(input), lexems(lexems), length(0), offset(0) {}

	void print(FILE *file) {
		if (!input.empty()) {	
			fprintf(file, "input: %s\n", input.c_str());

			fprintf(file, " Мітка   Мнемокод    1-ий операнд      2-ий операнд\n");
			fprintf(file, " індекс   індекс   індекс кількість  індекс кількість\n");

			int lindex = label.empty() ? -1 : (label[0].index + 1);
			int mindex = mnemocode.empty() ? -1 : (mnemocode[0].index + 1);
			int o1index = operands.size() > 0 ? (operands[0].lexems[0].index + 1) : -1;
			int o1count = operands.size() > 0 ? operands[0].lexems.size() : 0;
			int o2index = operands.size() > 1 ? (operands[1].lexems[0].index + 1) : -1;
			int o2count = operands.size() > 1 ? operands[1].lexems.size(): 0;
			fprintf(file, " %6i  %8i  %6i %9i  %6i %9i\n\n", lindex, mindex, o1index, o1count, o2index, o2count);	
			int index = 0;
			for (Lexem &lexem : lexems) {
				fprintf(file, "%-2d | %11s | %2i | %16s |\n", ++index, lexem.text.c_str(), lexem.text.size(), lexemInfo[lexem.type].c_str());
			}
			fprintf(file, "\n");
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
struct Look1 {
	map<string, unsigned> segment_table;
	map<string, Variable> variables;
	map<string, string> assume_table;
	vector<Sentence> sentences;
	string default_segment;
	string filename;
	unsigned offset;
	string segment;
	FILE *out;
	bool printed;

	vector<Lexem> divide(const string &);
	void run(const string &);
	void printOffset();
	void print();

	void parse(const string &);

	void printSegments();
	void printVariables();

	vector<IF> ifTable;
};

vector<Lexem> Look1::divide(const string &input) {
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
				map<string, Variable>::iterator equ = variables.find(lexem.text);
				if ((equ != variables.end()) && (equ->second.type.compare("NUMBER") == 0)) {
					lexem.type = DEC_CONST;
					lexem.text = to_string(equ->second.value);
				}
			}

			lexems.push_back(lexem);
		}
	}
	return lexems;
}

void Look1::printOffset() {
	if (printed) return;
	printed = true;
	fprintf(out, " %2X ", offset);
}

void Look1::parse(const string &input) {
	vector<Lexem> lexems = divide(input);
	Sentence sentence(input, lexems);
	Matcher matcher(lexems);
	printed = false;

	if (matcher.compare(USER_IDENT)) {
		sentence.label.push_back(matcher.get());
		if (matcher.compare(":")) {
			Variable variable;
			variable.type = "L NEAR";
			variable.value = offset;
			variable.segment = segment;
			variables[sentence.label[0].text] = variable;
			printOffset();

			sentence.label.push_back(matcher.get());
		}
	}

	if (matcher.compare(DIRECTIVE) || matcher.compare(COMMAND) || matcher.compare(DATA_TYPE)) {
		sentence.mnemocode.push_back(matcher.get());

		while (matcher.has()) {
			vector<Lexem> lexems;
			while (matcher.has() && !matcher.confirm(",")) {
				lexems.push_back(matcher.get());
			}
			Operand operand(lexems);

			Matcher matcher(lexems);

			if (matcher.compare(PTR_TYPE)) {
				string ptr = matcher.get().text;

				if (matcher.confirm(PTR_PTR)) {
					if (ptr.compare("byte") == 0) {
						operand.table[1].set(true, 1, -1);
					} else if (ptr.compare("word") == 0) {
						operand.table[1].set(true, 2, -1);
					} else if (ptr.compare("dword") == 0) {
						operand.table[1].set(true, 4, -1);
					}
				}
			}

			if (matcher.compare(REG_8)) {
				operand.table[0].set(true, 8, matcher.get());
			} else if (matcher.compare(REG_32)) {
				operand.table[0].set(true, 32, matcher.get());
			} else if (matcher.compare(BIN_CONST)) {
				operand.table[9].set(true, stol(matcher.get().text, 0, 2), -1);
			} else if (matcher.compare(DEC_CONST)) {
				operand.table[9].set(true, stol(matcher.get().text, 0, 10), -1);
			} else if (matcher.compare(HEX_CONST)) {
				operand.table[9].set(true, stol(matcher.get().text, 0, 16), -1);
			} else if (matcher.compare(STR_CONST)) {
				operand.table[9].set(true, matcher.get().text, -1);
			}

			if (matcher.compare(SEG_REG)) {
				Lexem &sreg = matcher.get();
				if (matcher.confirm(":")) {
					operand.table[2].set(true, sreg, -1);
				}
			}

			if (matcher.compare(USER_IDENT)) {
				Lexem &ident = matcher.get();
				if (variables.find(ident.text) != variables.end()) {
					operand.table[3].set(true, ident, -1);
				} else {
					operand.table[4].set(true, ident, -1);
				}
			}

			if (matcher.confirm("[")) {
				if (matcher.compare(REG_32)) {
					operand.table[5].set(true, matcher.get(), 32);
					if (matcher.confirm("+")) {
						if (matcher.compare(REG_32)) {	
							Lexem &reg = matcher.get();
							operand.table[6].set(true, reg, 32);
							operand.table[7].set(true, reg, 1);
						}
					}	
				}
				matcher.confirm("]");
			}

			sentence.operands.push_back(operand);
		}

		if (sentence.mnemocode[0].text.compare("if") == 0) {
			IF context;
			context.value = sentence.operands[0].table[9].first.number;
			ifTable.push_back(context);
			if (!context.value) return;
		} else if (sentence.mnemocode[0].text.compare("else") == 0) {
			IF &context = ifTable.back();
			context.value = !context.value;
			if (!context.value) return;
		} else if (sentence.mnemocode[0].text.compare("endif") == 0) { 
			ifTable.pop_back();
		} else if (!ifTable.empty() && !ifTable.back().value) {
			return;
		} else if (sentence.mnemocode[0].text.compare("equ") == 0) {
			fprintf(out, " = ");
			Variable variable;
			variable.type = "NUMBER";
			variable.segment = "";
			if (sentence.operands[0].table[9].first.isNumber()) {
				variable.value = sentence.operands[0].table[9].first.number;	
				fprintf(out, "%2X", variable.value);
			} else variable.value = 0;
			variables[sentence.label[0].text] = variable;
		} else if (sentence.mnemocode[0].text.compare("assume") == 0) {
			for (Operand &operand : sentence.operands) {
				assume_table[operand.table[2].first.lexem.text] = operand.table[4].first.lexem.text;
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
			Variable variable;
			variable.value = offset;
			variable.segment = segment;

			if (sentence.mnemocode[0].text.compare("db") == 0) {
				variable.type = "L BYTE";
				if (sentence.operands[0].table[9].first.isString()) {
					sentence.length = sentence.operands[0].table[9].first.text.size();
				} else sentence.length = 1; 
			} else if (sentence.mnemocode[0].text.compare("dw") == 0) {
				variable.type = "L WORD";
				sentence.length = 2;
			} else if (sentence.mnemocode[0].text.compare("dd") == 0) {
				variable.type = "L DWORD";
				sentence.length = 4;
			} 

			if (sentence.label.size() == 1)
				variables[sentence.label[0].text] = variable;
		} else {
			printOffset();
			if (sentence.mnemocode[0].text.compare("aaa") == 0) {
				sentence.length = 1;
			} else if (sentence.mnemocode[0].text.compare("inc") == 0) {
				if (sentence.operands[0].isREG()) {
					if (sentence.operands[0].table[0].first.number == 8) {
						sentence.length = 2;
					} else if (sentence.operands[0].table[0].first.number == 32) {
						sentence.length = 1;
					}
				}
			} else if (sentence.mnemocode[0].text.compare("div") == 0) {
				if (sentence.operands[0].isMEM()) {
					sentence.length = 2;

					if (sentence.operands[0].table[3].present || sentence.operands[0].table[4].present) {
						sentence.length += 4;
					}

					if (sentence.operands[0].table[3].present && (variables.find(sentence.operands[0].table[3].first.toString())->second.segment.compare(default_segment) != 0)) {
						sentence.length += 1;
					} else if (sentence.operands[0].table[2].present) {
						if (default_segment.compare(sentence.operands[0].table[2].first.lexem.text) != 0) {
							sentence.length += 1;
						}
					} 

					if (sentence.operands[0].table[6].present) {
						sentence.length += 1;
					}
				}
			} else if (sentence.mnemocode[0].text.compare("add") == 0) {
				if (sentence.operands[0].isREG() && sentence.operands[1].isREG()) {
					sentence.length = 2;
				}
			} else if (sentence.mnemocode[0].text.compare("cmp") == 0) {
				if (sentence.operands[0].isREG() && sentence.operands[1].isMEM()) {
					sentence.length = 2;

					if (sentence.operands[1].table[3].present || sentence.operands[1].table[4].present) {
						sentence.length += 4;
					}

					if (sentence.operands[1].table[2].present) {
						if (default_segment.compare(sentence.operands[1].table[2].first.lexem.text) != 0) {
							sentence.length += 1;
						}
					}

					if (sentence.operands[1].table[6].present) {
						sentence.length += 1;
					}
				}
			} else if (sentence.mnemocode[0].text.compare("and") == 0) {
				if (sentence.operands[0].isMEM() && sentence.operands[1].isREG()) {
					sentence.length = 2;

					if (sentence.operands[0].table[3].present || sentence.operands[0].table[4].present) {
						sentence.length += 4;
					}

					if (sentence.operands[0].table[2].present) {
						if (default_segment.compare(sentence.operands[0].table[2].first.lexem.text) != 0) {
							//sentence.length += 1;
						}
					}

					if (sentence.operands[0].table[6].present) {
						sentence.length += 1;
					}
				}
			} else if (sentence.mnemocode[0].text.compare("imul") == 0) {
				if (sentence.operands[0].isREG() && sentence.operands[1].isIMM()) {
					if (sentence.operands[0].table[0].first.number == 8) {
					 	sentence.length = 3;
					} else if (sentence.operands[0].table[0].first.number == 32) {
					 	sentence.length = 6;
					}
				}
			} else if (sentence.mnemocode[0].text.compare("or") == 0) {
				if (sentence.operands[0].isMEM() && sentence.operands[1].isIMM()) {
					sentence.length = 2;

					if (sentence.operands[0].table[3].present || sentence.operands[0].table[4].present) {
						sentence.length += 4;
					}

					long number = sentence.operands[1].table[9].first.number;
					int size = number == 0 ? 0 : (((number >= -128) && (number <= 127)) ? 1 : 4);
					if (sentence.operands[0].table[1].present) {
						if (sentence.operands[0].table[1].first.number == 1) {
							sentence.length += 1;
						} else if (sentence.operands[0].table[1].first.number == 4) {
							sentence.length += size;
						}
					} else {
						sentence.length += size;
					}

					if (sentence.operands[0].table[2].present) {
						if (default_segment.compare(sentence.operands[0].table[2].first.lexem.text) != 0) {
							sentence.length += 1;
						}
					}

					if (sentence.operands[0].table[6].present) {
						sentence.length += 1;
					}		
				}
			} else if (sentence.mnemocode[0].text.compare("jbe") == 0) {
				if (sentence.operands[0].isLabel()) {
					if (sentence.operands[0].table[3].first.isToken()) {
						Variable &variable = variables.find(sentence.operands[0].table[3].first.lexem.text)->second;
						long delta = offset - variable.value;
						if (delta > 0xFF) sentence.length = 6;
						else sentence.length = 2;
					} else {
						sentence.length = 6;
					}
				}
			}
		}
	}

	sentence.offset = offset;
	offset += sentence.length;
	sentences.push_back(sentence);	
	fprintf(out, "\t\t%s\n", sentence.input.c_str());
}

void Look1::run(const string &filepath) {
	size_t index = filepath.find_last_of(".");
	filename = index != string::npos ? filepath.substr(0, index) : filepath;

	out = fopen((filename + ".lst").c_str(), "w");
	ifTable.clear();

	ifstream file(filename + ".asm");
	string line;

	while (getline(file, line)) {
		parse(line);
	}
	file.close();

	printSegments();
	printVariables();

	fprintf(out, "\n@FILENAME  . . . . . . . . . . .\tTEXT  %s\n", filename.c_str());
	fclose(out);
}

void Look1::printSegments() {
	fprintf(out, "\n\n                І м ' я         	Розмір	Довжина\n\n");
	for (auto &segment : segment_table) {
		fprintf(out, "%s%.*s\t%-7s\t%-.4X\n", segment.first.c_str(), 32 - segment.first.size(), " . . . . . . . . . . . . . . . .", "32 Bit", segment.second);
	}
}

void Look1::printVariables() {
	fprintf(out, "\nСимволи:\n                І м ' я         	Тип 	 Дані	 Атрибути\n\n");
	for (auto &variable : variables) {
		fprintf(out, "%s%.*s\t%-7s\t%-.4X\t%s\n", variable.first.c_str(), 32 - variable.first.size(), " . . . . . . . . . . . . . . . .", variable.second.type.c_str(), variable.second.value, variable.second.segment.c_str());
	}
}

void Look1::print() {
	FILE *file = fopen((filename + "_lexical.txt").c_str(), "w");
	for (Sentence &sentence : sentences) {
		sentence.print(file);
	}
	fclose(file);
}

int main() {
	Look1 view;
	view.run("test.asm");
	view.print();
}
