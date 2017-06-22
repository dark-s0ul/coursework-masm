import re, string

def enum(**args):
	return type('Enum', (), args)

LexemType = enum(
	UNKNOWN = "unknown",
	ONE_CHAR = "one char",
	STR_CONST = "string",
	BIN_CONST = "binary",
	DEC_CONST = "decimal",
	HEX_CONST = "heximal",
	USER_IDENT = "identifier",
	DIRECTIVE = "directive",
	DATA_TYPE = "data type",
	PTR_TYPE = "ptr type",
	OPERATOR = "operator",
	REG_8 = "8-bit register",
	REG_16 = "16-bit register",
	REG_32 = "32 bit register",
	SEG_REG = "segment register",
	COMMAND = "command"
)

def LT(text, default):
	if text in ('end', 'segment', 'ends', 'assume'):
		return LexemType.DIRECTIVE
	elif text in ('db', 'dw', 'dd'):
		return LexemType.DATA_TYPE
	elif text in ('byte', 'dword', 'word'):
		return LexemType.PTR_TYPE
	elif text in ('ptr'):
		return LexemType.OPERATOR
	elif text in ('ah', 'al', 'bh', 'bl', 'dh', 'dl', 'ch', 'cl'):
		return LexemType.REG_8
	elif text in ('ax', 'bx', 'dx', 'cx', 'si', 'di', 'sp', 'bp'):
		return LexemType.REG_16
	elif text in ('eax', 'ebx', 'edx', 'ecx', 'esi', 'ebp', 'esp', 'edi'):
		return LexemType.REG_32
	elif text in ('cs', 'ds', 'es', 'fs', 'gs', 'ss'):
		return LexemType.SEG_REG
	elif text in ('sti', 'dec', 'add', 'cmp', 'or', 'and', 'and', 'jle', 'jmp'):
		return LexemType.COMMAND
	else:
		return default

SegmentCode = { "cs": "2E", "ds": "3E", "es": "26", "fs": "64", "gs": "65", "ss": "36" }

def isxdigit(c):
	return c.isdigit() or (c.lower() in ('a', 'b', 'c', 'd', 'e', 'f'))

def isbinary(c):
	return c in ('0', '1')

def issymbol(c):
	return ('+', '*', ':', '[', ']', ',')

def isquote(c):
	return c in ('\'', '"')

def RS(reg):
	if reg.text in ("esp", "ebp", "sp", "bp"):
		return "ss"
	elif reg.type in (LexemType.REG_32, LexemType.REG_16):
		return "ds"
	return ""

class Lexem:
	type = LexemType.UNKNOWN
	index = None
	text = ""

class Sentence:
	printable = False
	source = ""
	prefix = ""
	bytes = ""
	offset = 0
	length = 0
	label = -1
	mnemo = -1
	operands = []

	def __init__(self, source, lexems):
		self.source = source
		self.lexems = lexems
		self.operands = []
		i = 0
		if (i < len(self.lexems)) and (self.lexems[i].type == LexemType.USER_IDENT):
			self.label = i + 1
			i += 1
			if (i < len(self.lexems)) and (self.lexems[i].text == ":"):
				i += 1
			
		if (i < len(self.lexems)) and (self.lexems[i].type in (LexemType.DIRECTIVE, LexemType.COMMAND, LexemType.DATA_TYPE)):
			self.mnemo = i + 1
			i += 1

		while i < len(self.lexems):
			index = i
			while (i < len(self.lexems)):
				if self.lexems[i].text == ",":
					i += 1
					break
				i += 1
			self.operands.append([index + 1, i - index])

	def printLexical(self):
		if self.source and self.source.strip():
			print("> %s" % self.source.rstrip("\n"))
			print(" Мітка   Мнемокод    1-ий операнд      2-ий операнд\n");
			print(" індекс   індекс   індекс кількість  індекс кількість\n");

			index1 = self.operands[0][0] if len(self.operands) > 0 else -1
			count1 = self.operands[0][1] if len(self.operands) > 0 else 0
			index2 = self.operands[1][0] if len(self.operands) > 1 else -1
			count2 = self.operands[1][1] if len(self.operands) > 1 else 0

			print(" %6i  %8i  %6i %9i  %6i %9i\n" % (self.label, self.mnemo, index1, count1, index2, count2));
			
			for lexem in self.lexems:
				print("| %2d | %-11s | %i | %16s |" % (lexem.index, lexem.text, len(lexem.text), lexem.type))
			print()

	def printOffsets(self):
		if self.printable:
			print(" %.4X " % self.offset, end = '')
		elif self.prefix and self.prefix.strip():
			print(self.prefix, end = '')
		else:
			print("      ", end = '')

		if self.bytes and self.bytes.strip():
			print(" %s " % self.bytes, end = '')

		print("\t\t%s" % self.source.rstrip("\n"))

class Symbol:
	segment = ""
	value = ""
	type = ""

class Operand:
	reg = None
	ptr = None
	seg = None
	ident = None
	base = None
	index = None
	disp = 0
	imm = None

class Lexer:
	current_line = 0
	sentences = []
	segments = {}
	filename = ""
	segment = ""
	symbols = {}
	assume = {}
	offset = 0

	def lex(self, line):
		replaced = line
		lexems = []
		i = 0
		index = 0
		while i < len(line):
			if line[i] == ';':
				break
			elif line[i].isspace():
				i += 1
			else:
				index += 1
				lexem = Lexem()
				lexem.index = index
				lexem.pos = i
				if line[i].isalpha():
					while (i < len(line)) and (line[i].isalpha() or line[i].isdigit()) :
						lexem.text += line[i]
						i += 1
					lexem.text = lexem.text.lower()
					lexem.type = LT(lexem.text, LexemType.USER_IDENT)
				elif line[i].isdigit():
					numb_er = False
					while (i < len(line)) and isxdigit(line[i]):
						lexem.text += line[i]
						i += 1
					lexem.text = lexem.text.lower()
					if (i < len(line)) and (line[i] == 'h'):
						i += 1
						for j in range (0, len(lexem.text)):
							if not isxdigit(lexem.text[j]):
								print("Wrong number in line " + str(self.current_line))
								exit(1)
						lexem.type = LexemType.HEX_CONST
					elif lexem.text[len(lexem.text) - 1] == "b":
						for j in range (0, len(lexem.text) - 1):
							if not isbinary(lexem.text[j]):
								print("Wrong number in line " + str(self.current_line))
								exit(1)
						lexem.type = LexemType.BIN_CONST
						lexem.text = lexem.text[:-1]
					else:
						for j in range (0, len(lexem.text)):
							if not lexem.text[j].isdigit():
								print("Wrong number in line " + str(self.current_line))
								exit(1)
						lexem.type = LexemType.DEC_CONST
				elif isquote(line[i]):
					quote = line[i]
					i += 1
					while (i < len(line)) and (line[i] != quote) and (line[i] != '\n'):
						lexem.text += line[i]
						i += 1

					if (i >= len(line)) or (line[i] != quote):
						print("Wrong string in line " + str(self.current_line))
						exit(1)
					lexem.type = LexemType.STR_CONST
					i += 1	
				elif issymbol(line[i]):
					lexem.text = line[i]
					lexem.type = LexemType.ONE_CHAR
					i += 1
				else:
					print("Illegal symbol in line " + str(self.current_line) + " position " + str(i))
					print("Symbol: " + line[i])
					exit(1)
				
				lexems.append(lexem)
		return lexems

	def parseREG(self, lexems, i):
		operand = Operand()
		if i < len(lexems):
			if lexems[i].type in (LexemType.REG_8, LexemType.REG_16, LexemType.REG_32, LexemType.SEG_REG):
				operand.reg = lexems[i]
			else:
				print("Illegal operand type on line " + str(self.current_line))
				exit(1)
			i += 1
		else:
			print("Missing operand on line " + str(self.current_line))
			exit(1)
		return [i, operand]

	def parseIMM(self, lexems, i):
		operand = Operand()
		if i < len(lexems):
			if lexems[i].type == LexemType.BIN_CONST:
				operand.imm = int(lexems[i].text, 2)
			elif lexems[i].type == LexemType.DEC_CONST:
				operand.imm = int(lexems[i].text, 10)
			elif lexems[i].type == LexemType.HEX_CONST:
				operand.imm = int(lexems[i].text, 16)
			else:
				print("Illegal operand type on line " + str(self.current_line))
				exit(1)
			i += 1
		else:
			print("Missing operand on line " + str(self.current_line))
			exit(1)
		return [i, operand]

	def parseMEM(self, lexems, i):
		operand = Operand()
		if i < len(lexems):
			if lexems[i].type == LexemType.PTR_TYPE:
				operand.ptr = lexems[i].text
				i += 1
				if (i < len(lexems)) and (lexems[i].type == LexemType.OPERATOR):
					i += 1
				else:
					print("Missing PTR operator on line " + str(self.current_line))
					exit(1)

			if i < len(lexems):
				if lexems[i].type == LexemType.SEG_REG:
					operand.seg = lexems[i].text
					i += 1
					if (i < len(lexems)) and (lexems[i].text == ":"):
						i += 1
					else:
						print("Missing ':' operator on line " + str(self.current_line))
						exit(1)
			else:
				print("Illegal operand on line " + str(self.current_line))
				exit(1)

			if (i < len(lexems)) and (lexems[i].type == LexemType.USER_IDENT):
				operand.ident = lexems[i].text
				i += 1

			if (i < len(lexems)) and (lexems[i].text == "["):
				i += 1
				if (i < len(lexems)) and (lexems[i].type in (LexemType.REG_16, LexemType.REG_32)):
					operand.base = lexems[i]
					i += 1
					if (i < len(lexems)) and (lexems[i].text == "+"):
						i += 1
						if (i < len(lexems)) and (lexems[i].type in (LexemType.REG_16, LexemType.REG_32)):
							operand.index = lexems[i]
							i += 1
							if (i < len(lexems)) and (lexems[i].text == "]"):
								i += 1
							else:
								print("Missing ']' operator on line " + str(self.current_line))
								exit(1)
						else:
							print("Missing index register on line" + str(self.current_line))
							exit(1)
					else:
						print("Missing '+' operator on line " + str(self.current_line))
						exit(1)
				else:
					print("Expected base register on line " + str(self.current_line))
					exit(1)
		else:
			print("Missing operand on line " + str(self.current_line))
			exit(1)
		return [i, operand]

	def sizeOfIMM(self, ptr, imm):
		if ptr:
			if (ptr == "byte") or (ptr == LexemType.REG_8):
				if imm in range(-256, 256):
					return 1
				else:
					print("Value out of range at line " + str(self.current_line))
					exit(1)
			elif (ptr == "word") or (ptr == LexemType.REG_16):
				if imm in range(-65536, 65536):
					if imm in range(-128, 128):
						return 1
					else:
						return 2
				else:
					print("Value out of range at line " + str(self.current_line))
					exit(1)

		if imm in range(-128, 128):
			return 1
		else:
			return 4

	def parseOperands(self, func1, func2, lexems, i):
		operand1 = None
		operand2 = None
		if i < len(lexems):
			operand1 = func1(lexems, i)
			i = operand1[0]
			if i < len(lexems) and (lexems[i].text == ","):
				i += 1
				operand2 = func2(lexems, i)
				i = operand2[0]
			else:
				print("Expected ',' operator on line " + str(self.current_line))
		else:
			print("Missing operand on line " + str(self.current_line))
			exit(1)
		return [i, operand1[1], operand2[1]]

	def parseIdent(self, lexems, i):
		operand = Operand()
		if i < len(lexems):
			if lexems[i].type == LexemType.USER_IDENT:
				operand.ident = lexems[i].text
			else:
				print("Illegal operand type on line " + str(self.current_line))
				exit(1)
			i += 1
		else:
			print("Missing operand on line " + str(self.current_line))
			exit(1)
		return [i, operand]

	def parse(self, path):
		self.filename = path.split(":")[0]
		for line in open(path, 'r'):
			sentence = Sentence(line, self.lex(line))
			lexems = sentence.lexems
			self.current_line += 1
			ident = None
			mnemo = None
			i = 0
			if (i < len(lexems)) and (lexems[i].type == LexemType.USER_IDENT):
				label = lexems[i]
				i += 1
				if (i < len(lexems)):
					if (lexems[i].text == ":"):
						i += 1
						sentence.printable = True
						symbol = Symbol()
						symbol.segment = self.segment
						symbol.value = "%.4X" % self.offset
						symbol.type = "L NEAR"
						self.symbols[label.text] = symbol
					else:
						ident = label
						
			if (i < len(lexems)) and (lexems[i].type in (LexemType.DIRECTIVE, LexemType.COMMAND, LexemType.DATA_TYPE)):
				mnemo = lexems[i]
				i += 1

			if mnemo:
				if ident:
					if mnemo.text == "segment":
						sentence.printable = True
						self.offset = 0
						self.segment = ident.text
					elif mnemo.text == "ends":
						sentence.printable = True
						self.segments[ident.text] = self.offset
					elif mnemo.type == LexemType.DATA_TYPE:
						if mnemo.text == "db":
							sentence.printable = True
							sentence.length = 1

							symbol = Symbol()
							symbol.type = "L BYTE"
							symbol.value = "%.4X" % self.offset
							symbol.segment = self.segment

							if i < len(lexems):
								if (lexems[i].type == LexemType.STR_CONST):
									sentence.length = len(lexems[i].text)
								i += 1
							else:
								print("Missing operand on line " + str(self.current_line))
								exit(1)

							self.symbols[ident.text] = symbol
						elif mnemo.text == "dw":
							sentence.printable = True
							sentence.length = 2

							symbol = Symbol()
							symbol.type = "L WORD"
							symbol.value = "%.4X" % self.offset
							symbol.segment = self.segment
							if i < len(lexems):
								i += 1
							else:
								print("Missing operand on line " + str(self.current_line))
								exit(1)

							self.symbols[ident.text] = symbol
						elif mnemo.text == "dd":
							sentence.printable = True
							sentence.length = 4

							symbol = Symbol()
							symbol.type = "L DWORD"
							symbol.value = "%.4X" % self.offset
							symbol.segment = self.segment

							if i < len(lexems):
								i += 1
							else:
								print("Missing operand on line " + str(self.current_line))
								exit(1)

							self.symbols[ident.text] = symbol
					else:
						print(line)
						print("Exprected instruction or directive")
						exit(1)
				else:
					if mnemo.text == "end":
						break
					elif mnemo.text == "assume":
						while i < len(lexems):
							sreg = None
							seg = None
							if lexems[i].type == LexemType.SEG_REG:
								sreg = lexems[i]
								i += 1
							else:
								print("Exprected segment register on line " + str(self.current_line))
								exit(1)
							if (i < len(lexems)) and (lexems[i].text == ":"):
								i += 1
							else:
								print("Missing ':' on line " + str(self.current_line))
								exit(1)

							if (i < len(lexems)) and (lexems[i].type == LexemType.USER_IDENT):
								seg = lexems[i]
								i += 1
							else:
								print("Exprected identifier on line" + str(self.current_line))
								exit(1)

							if (i < len(lexems)) and (lexems[i].text == ","):
								i += 1
							else:
								break
							self.assume[sreg.text] = seg.text
					elif mnemo.type == LexemType.COMMAND:
						sentence.printable = True
						instr = mnemo
						
						if instr.text == "sti":
							sentence.length = 1
						elif instr.text == "dec":
							operand = self.parseREG(lexems, i)
							i = operand[0]
							reg = operand[1].reg

							if reg.type == LexemType.REG_16:
								sentence.length = 1
							elif reg.type in (LexemType.REG_8, LexemType.REG_32):
								sentence.length = 2
							else:
								print("Illegal use of register on line " + str(self.current_line))
								exit(1)
						elif instr.text == "add":
							operands = self.parseOperands(self.parseREG, self.parseMEM, lexems, i)
							i = operands[0]

							ptr = operands[2].ptr
							seg = operands[2].seg
							base = operands[2].base
							index = operands[2].index
							ident = operands[2].ident

							if not ident:
								print("Illegal operand type at line " + str(self.current_line))
								exit(1)
							elif not ident in self.symbols:
								print("Unknown symbol at line " + str(self.current_line))
								exit(1)

							sentence.length = 2

							if seg and (seg != RS(base)):
								sentence.length += 1
							elif RS(base) != "ds":
								sentence.length += 1
							else:
								symbol = self.symbols[ident]
								sreg = next((k for k, v in self.assume.items() if v == symbol.segment), None)
								if sreg:
									if sreg != "ds":
										sentence.length += 1

							if (ptr and (ptr == "dword")) or (operands[1].reg.type == LexemType.REG_32):
								sentence.length += 1

							if (base.type == LexemType.REG_32) or (index.type == LexemType.REG_32):
								sentence.length += 1

							if (base.text in ("esi", "edi")) or (index.text in ("esi", "edi")):
								sentence.length += 1

							if (base.type == LexemType.REG_32) or (index.type == LexemType.REG_32):
								sentence.length += 4
							else:
								sentence.length += 2
						elif instr.text == "cmp":
							operands = self.parseOperands(self.parseREG, self.parseREG, lexems, i)
							i = operands[0]

							sentence.length = 2

							if (operands[1].reg.type == LexemType.REG_32) and (operands[2].reg.type == LexemType.REG_32):
								sentence.length += 1

							if i < len(lexems):
								print("Extra characters on line " + str(self.current_line))
								exit(1)
						elif instr.text == "or":
							operands = self.parseOperands(self.parseMEM, self.parseIMM, lexems, i)
							i = operands[0]

							ptr = operands[1].ptr
							seg = operands[1].seg
							base = operands[1].base
							index = operands[1].index
							ident = operands[1].ident
							req = self.sizeOfIMM(ptr, operands[2].imm & 0xFFFFFFFF)

							if not ident:
								print("Illegal operand type at line " + str(self.current_line))
								exit(1)
							elif not ident in self.symbols:
								print("Unknown symbol at line " + str(self.current_line))
								exit(1)

							sentence.length = 2

							if seg and (seg != RS(base)):
								sentence.length += 1
							elif RS(base) != "ds":
								sentence.length += 1
							else:
								symbol = self.symbols[ident]
								sreg = next((k for k, v in self.assume.items() if v == symbol.segment), None)
								if sreg:
									if sreg != "ds":
										sentence.length += 1

							if (base.type == LexemType.REG_32) or (index.type == LexemType.REG_32):
								sentence.length += 1

							if ptr and (ptr == "dword") or (self.symbols[ident].type == "L DWORD"):
								sentence.length += 1

							if (base.text in ("esi", "edi")) or (index.text in ("esi", "edi")):
								sentence.length += 1

							if (base.type == LexemType.REG_32) or (index.type == LexemType.REG_32):
								sentence.length += 4
							else:
								sentence.length += 2

							if req == 1:
								sentence.length += 1
							elif req == 2:
								sentence.length += 2
							else:
								sentence.length += 4
						elif instr.text == "and":
							operands = self.parseOperands(self.parseREG, self.parseIMM, lexems, i)
							i = operands[0]
							reg = operands[1].reg
							req = self.sizeOfIMM(reg.type, operands[2].imm & 0xFFFFFFFF)
							sentence.length = 2

							if reg.type == LexemType.REG_32:
								sentence.length += 1

							if req == 1:
								sentence.length += 1
							elif req == 2:
								sentence.length += 2
							else:
								sentence.length += 4
						elif instr.text == "jle":
							operand = self.parseIdent(lexems, i)
							i = operand[0]
							sentence.length = 2
						elif instr.text == "jmp":
							operand = self.parseMEM(lexems, i)
							i = operand[0]

							ptr = operand[1].ptr
							seg = operand[1].seg
							base = operand[1].base
							index = operand[1].index
							ident = operand[1].ident
							req = self.sizeOfIMM(ptr, operands[2].imm & 0xFFFFFFFF)

							if not ident:
								print("Illegal operand type at line " + str(self.current_line))
								exit(1)
							elif not ident in self.symbols:
								print("Unknown symbol at line " + str(self.current_line))
								exit(1)

							sentence.length = 2

							if base and seg and (seg != RS(base)):
								sentence.length += 1
							elif base and (RS(base) != "ds"):
								sentence.length += 1
							else:
								symbol = self.symbols[ident]
								sreg = next((k for k, v in self.assume.items() if v == symbol.segment), None)
								if sreg:
									if sreg != "ds":
										sentence.length += 1

							if base and ((base.type == LexemType.REG_32) or (index.type == LexemType.REG_32)):
								sentence.length += 1

							if ptr and (ptr == "dword") or (self.symbols[ident].type == "L DWORD"):
								sentence.length += 1

							if base and ((base.text in ("esi", "edi")) or (index.text in ("esi", "edi"))):
								sentence.length += 1

							if base:
								if (base.type == LexemType.REG_32) or (index.type == LexemType.REG_32):
									sentence.length += 4
								else:
									sentence.length += 2
							else:
								type = self.symbols[ident].type
								if type == "L WORD":
									sentence.length += 2
								elif type == "L DWORD":
									sentence.length += 4

						while i < len(lexems):
							i += 1
			elif ident or (i < len(lexems)):
				print(line)
				print("Exprected instruction or directive")
				exit(1)

			if i < len(lexems):
				print("Extra characters on line " + str(self.current_line))
				exit(1)
			
			sentence.offset = self.offset
			self.offset += sentence.length
			self.sentences.append(sentence)

	def printLexical(self):
		for sentence in self.sentences:
			sentence.printLexical()

	def printListing(self):
		for sentence in self.sentences:
			sentence.printOffsets()

		print("\n\n                N a m e         	Size  	Length\n");
		for segment in sorted(self.segments):
			print("%-32s\t%-7s\t%-.4X" % (segment.upper(), "16 Bit", self.segments[segment]))
		
		print("\n\nSymbols:\n                N a m e         	Type	Value	Attr\n");
		for symbol in sorted(self.symbols):
			print("%-32s\t%-7s\t%-s\t%s" % (symbol.upper(), self.symbols[symbol].type, self.symbols[symbol].value, self.symbols[symbol].segment.upper()))

if __name__=="__main__":
	lexer = Lexer()
	lexer.parse("test.asm")
	lexer.printLexical()
	print()
	lexer.printListing()