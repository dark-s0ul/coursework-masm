import Foundation

enum LexemType {
	case UNKNOWN,
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
	var type : LexemType;
	var text : String;
};

let lexemInfo : [LexemType: String] = [
	LexemType.UNKNOWN: "unknown",
	LexemType.ONE_CHAR: "one char",
	LexemType.USER_IDENT: "identifier",
	LexemType.STR_CONST: "string",
	LexemType.BIN_CONST: "binary",
	LexemType.DEC_CONST: "decimal",
	LexemType.HEX_CONST: "hexadecimal",
	LexemType.DIRECTIVE: "directive",
	LexemType.DATA_TYPE: "data type",
	LexemType.PTR_TYPE: "ptr type",
	LexemType.PTR_PTR: "operator",
	LexemType.SEG_REG: "segment register",
	LexemType.REG_32: "32-bit register",
	LexemType.REG_8: "8-bit register",
	LexemType.COMMAND: "command"
];

let lexemType : [String: LexemType] = [
	"assume": LexemType.DIRECTIVE,
	"end": LexemType.DIRECTIVE,
	"segment": LexemType.DIRECTIVE,
	"ends": LexemType.DIRECTIVE,
	"equ": LexemType.DIRECTIVE,
	"if": LexemType.DIRECTIVE, 
	"else": LexemType.DIRECTIVE,
	"endif": LexemType.DIRECTIVE,

	"db": LexemType.DATA_TYPE,
	"dw": LexemType.DATA_TYPE,
	"dd": LexemType.DATA_TYPE,

	"byte": LexemType.PTR_TYPE,
	"dword": LexemType.PTR_TYPE,
	"ptr": LexemType.PTR_PTR,

	"eax": LexemType.REG_32,
	"ebx": LexemType.REG_32,
	"ecx": LexemType.REG_32,
	"edx": LexemType.REG_32,
	"esi": LexemType.REG_32,
	"edi": LexemType.REG_32,
	"esp": LexemType.REG_32,
	"ebp": LexemType.REG_32,

	"ah": LexemType.REG_8,
	"bh": LexemType.REG_8,
	"ch": LexemType.REG_8,
	"dh": LexemType.REG_8,
	"al": LexemType.REG_8,
	"bl": LexemType.REG_8,
	"cl": LexemType.REG_8,
	"dl": LexemType.REG_8,

	"cs": LexemType.SEG_REG,
	"ds": LexemType.SEG_REG,
	"ss": LexemType.SEG_REG,
	"es": LexemType.SEG_REG,
	"fs": LexemType.SEG_REG,
	"gs": LexemType.SEG_REG,

	"aaa": LexemType.COMMAND,
	"inc": LexemType.COMMAND,
	"div": LexemType.COMMAND,
	"add": LexemType.COMMAND,
	"cmp": LexemType.COMMAND,
	"and": LexemType.COMMAND,
	"imul": LexemType.COMMAND,
	"or": LexemType.COMMAND,
	"jbe": LexemType.COMMAND
];

func isSingleChar(_ c : Character) -> Bool {
	return (c == "+") || (c == "-") || (c == "*") || (c == ":") || (c == ",") || (c == "[") || (c == "]");
};

struct Value {
	var text : String?;
	var lexem : Lexem?;
	var number : Int64?;
	var type : Int8;

	init() {
		type = -1;
	}

	init(_ value : Int64) {
		number = value;
		type = 0; 
	}

	init(_ value : Lexem) {
		lexem = value;
		type = 1;
	}

	init(_ value : String) {
		text = value;
		type = 2;
	}

	func isNumber() -> Bool { return type == 0; }
	func isToken() -> Bool { return type == 1; }
	func isString() -> Bool { return type == 2; }

	func toString() -> String {
		switch (type) {
			case 0: return String(number!);
			case 1: return lexem!.text;
			case 2: return text!;
			default: return "";
		}
	}
};

struct Variable {
	var value : UInt;
	var segment : String;
	var type : String;
};

struct Matcher {
	var lexems : Array<Lexem> = Array<Lexem>(); 
	var index : Int;

	init(lexems : Array<Lexem>) {
		self.lexems = lexems;
		index = 0;
	}

	mutating func reset() { index = 0; }
	mutating func next() { 
		if (index < lexems.count) {
			index += 1;
		} 
	}

	func peek() -> Lexem {
		return index < lexems.count ? lexems[index] : Lexem(type: LexemType.UNKNOWN, text: "");
	}
	mutating func get() -> Lexem{
		let lexem : Lexem = peek();
		next();
		return lexem;
	}

	func match(_ text : String) -> Bool {
		return peek().text == text;
	}

	mutating func confirm(_ text : String) -> Bool {
		if (match(text)) {
			next();
			return true;
		} else {
			return false;
		}
	}

	func match(_ type : LexemType) -> Bool {
		return peek().type == type;
	}

	mutating func confirm(_ type : LexemType) -> Bool {
		if (match(type)) {
			next();
			return true;
		} else {
			return false;
		}
	}

	func has() -> Bool {
		return index < lexems.count;
	}
};

struct Operand {
	struct Element {
		var present : Bool;
		var first, second : Value;

		init() {
			present = false;
			first = Value(-1);
			second = Value(-1);
		}
	};

	var lexems : Array<Lexem>;
	var elements : Array<Element> = Array<Element>(repeating: Element(), count: 10);

	init(lexems : Array<Lexem>) {
		self.lexems = lexems;

		for i in 0..<10 {
			set(i, false, Value(-1), Value(-1));
		}
	}

	mutating func set(_ index : Int, _ present : Bool, _ first : Value, _ second : Value) {
		elements[index].present = present;
		elements[index].first = first;
		elements[index].second = second;
	}

	func get(index : Int) -> Element {
		return elements[index];
	}

	func isreg() -> Bool {
		for i in 1..<10 {
			if (elements[i].present) {
				return false;
			}
		}
		return elements[0].present;
	}

	func isimm() -> Bool {
		for i in 2..<9 {
			if (elements[i].present) {
				return false;
			}
		}
		return !elements[0].present && elements[9].present;
	}

	func ismem() -> Bool {
		if (elements[0].present || elements[9].present) {
			return false;
		}
		for i in 3..<9 {
			if (elements[i].present) {
				return true;
			}
		}
		return false;
	}

	func islabel() -> Bool {
		if (elements[0].present || elements[1].present) {
			return false;
		}
		for i in 5..<10 {
			if (elements[i].present) {
				return false;
			}
		}
		return elements[3].present || elements[4].present;
	}
};

prefix public func ++(x: inout Int) -> Int {
	x += 1;
	return x;
}

postfix public func ++(x: inout Int) -> Int {
	x += 1;
	return x - 1;
}

struct Sentence {
	var operands : Array<Operand> = Array<Operand>();
	var lexems : Array<Lexem>;
	var label : Lexem?;
	var ident : Lexem?;
	var mnemo : Lexem?;
	var length : UInt;
	var offset : UInt;
	var source : String;
	var printed : Bool;

	init(_ source : String, _ lexems : Array<Lexem>) {
		self.source = source;
		self.lexems = lexems;
		self.printed = false;
		self.length = 0;
		self.offset = 0;
	}

	func printSentence() {
		if (source.isEmpty) {	
			print("\(source)\n");

			print(" Мітка   Мнемокод    1-ий операнд      2-ий операнд\n");
			print(" індекс   індекс   індекс кількість  індекс кількість\n");

			var index = 0;
			let labelIndex = ((label != nil) || (ident != nil)) ? ++index : -1;
			let mnemoIndex = (mnemo != nil) ? ++index : -1;
			let operand1Index = operands.count > 0 ? ++index : -1;
			let operand1Count = operands.count > 0 ? operands[0].lexems.count : 0;
			let operand2Index = operands.count > 1 ? (operand1Index + operand1Count) : -1;
			let operand2Count = operands.count > 1 ? operands[1].lexems.count : 0;

			print(" %6i  %8i  %6i %9i  %6i %9i\n\n", labelIndex, mnemoIndex, operand1Index, operand1Count, operand2Index, operand2Count);	
			index = 0;
			for lexem in lexems {
				print("%-2d | %11s | %2i | %16s |\n", ++index, lexem.text, lexem.text.length, lexemInfo[lexem.type] ?? "unknown");
			} 
			print("\n");
		}
	}
};

func isspace(_ c : Character) -> Bool {
	return NSCharacterSet.whitespacesAndNewlines.contains(c);
}

func isdigit(_ c : Character) -> Bool {
	return c >= "0" && c <= "9";
}

func isxdigit(_ c : Character) -> Bool {
	return isdigit(c) || (c >= "a" && c <= "f") || (c >= "A" && c <= "F")
}

func isalpha(_ c : Character) -> Bool {
	return NSCharacterSet.letters.contains(c);
}

func isalnum(_ c : Character) -> Bool {
	return NSCharacterSet.alphanumerics.contains(c);
}

func tolower(_ c : Character) -> Character {
	return String(c).lowercased()[0];
}

extension String {
	var length : Int {
		return self.characters.count;
	}

	mutating func removeLast() {
		self = self.remove(at: self.index(before: self.endIndex));
	}

	subscript (i: Int) -> Character {
		return self[index(startIndex, offsetBy: i)];
	}
}

func +=(s: inout String, c: Character) -> String {
	s += String(c);
	return s;
}

func +(s : String, c : Character) -> String {
	let ss : String = s + String(c);
	return ss;
}

func readFile(_ path: String) -> Array<String> {
	do { 
		let contents : NSString = try NSString(contentsOfFile: path, encoding: NSASCIIStringEncoding);
		let trimmed : String = contents.trimmingCharacters(in:NSCharacterSet.whitespacesAndNewlines());
		let lines : Array<String> = NSString(string: trimmed).components(separatedBy: NSCharacterSet.newlines);
		return lines;
	} catch {
		print("Unable to read file: \(path)");
		return [String]();
	}
}

struct FirstView {
	var Segments : [String: UInt] = [:];
	var Variables : [String: Variable] = [:];
	var Assume : [String: String] = [:];
	var Sentences : Array<Sentence> = Array<Sentence>();
	var FileName : String;
	var Offset : UInt;
	var Segment : String;

	init()  {
		Offset = 0;
		FileName = "";
		Segment = "";
	}

	mutating func split(_ input : String) -> Array<Lexem> {
		var lexems : Array<Lexem> = Array<Lexem>();
		var i : Int = 0;
		while i < input.length {
			if (input[i] == ";") {
				break;
			} else if (isspace(input[i])) {
				i++;
			} else {
				var lexem : Lexem = Lexem(type: LexemType.UNKNOWN, text: "");
				if (isalpha(input[i])) {
					while ((i < input.length) && isalnum(input[i])) {
						lexem.text += String(input[i++]).lowercased();
					}
					lexem.type = lexemType[lexem.text] ?? LexemType.USER_IDENT;
				} else if (isdigit(input[i])) {
					while ((i < input.length) && isxdigit(input[i])) {
						lexem.text += String(input[i++]).lowercased();
					}

					if (String(input.characters.last!).lowercased() == "b") {
						lexem.type = LexemType.BIN_CONST;
						lexem.text.removeLast();
					} else if (String(input[i]).lowercased() == "h") {
						lexem.type = LexemType.HEX_CONST;
						i++;
					} else {
						if (String(input[i]).lowercased() == "d") {
							i++;
						}
						lexem.type = LexemType.DEC_CONST;
					}
				} else if (input[i] == "\"") {
					i++;
					while ((i < input.length) && (input[i] != "\"")) {
						lexem.text += String(input[i++]).lowercased();
					}
					i++;
					lexem.type = LexemType.STR_CONST;
				} else if (isSingleChar(input[i])) {
					lexem.text = String(input[i++]);
					lexem.type = LexemType.ONE_CHAR;
				} else {
					i++;
				}

				if (lexem.type == LexemType.USER_IDENT) {
					let equ : Variable? = Variables[lexem.text];
					if ((equ != nil) && (equ!.type == "NUMBER")) {
						lexem.type = LexemType.DEC_CONST;
						lexem.text = String(equ!.value);
					}
				}

				lexems.append(lexem);
			}
		}
		return lexems;
	}

	mutating func parse(_ text : String) {
		let lines = readFile("./\(text)");

		for line in lines {
			let sentence : Sentence = Sentence(line, split(line));
			Sentences.append(sentence);
		}
	}

	func printResult() {
		for sentence in Sentences {
			sentence.printSentence();
		}
	}
};

func main() {
	var firstView : FirstView = FirstView();
	firstView.parse("test.asm");
	firstView.printResult();
}