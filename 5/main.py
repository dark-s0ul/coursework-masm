import re, string

Reg_32 = ('eax', 'ebx', 'edx', 'ecx', 'esi', 'ebp', 'esp', 'edi')
Reg_16 = ('ax', 'bx', 'dx', 'cx', 'si', 'di', 'sp', 'bp')
Reg_8 = ('ah', 'al', 'bh', 'bl', 'dh', 'dl', 'ch', 'cl')
SegmentReg = ('cs', 'ds', 'es', 'fs', 'gs', 'ss')
Command = ('cld', 'pop', 'mul', 'mov', 'xor', 'sub', 'cmp', 'add', 'jc', 'jmp')
Directive = ('end', 'segment', 'equ', 'ends', 'assume', 'db', 'dw', 'dd')
TypeOperator = ('ptr')
TypeDefinitor = ('byte', 'dword', 'word')
Hex = ('a', 'b', 'c', 'd', 'e', 'f')
Binary = ('0', '1')
SingleSymbol = ('+', '*', ':', '[', ']', ',')
dictionary = {
	Reg_32: '32-bit register', 
	Reg_16: '16-bit register',
	Reg_8: '8-bit register', 
	SegmentReg: 'segment', 
	Command: 'command', 
	Directive: 'directive', 
	TypeOperator: 'type operator', 
	SingleSymbol: 'sigle char', 
	TypeDefinitor: 'type definitor',
}
SegmentCode = { "cs": "2E", "ds": "3E", "es": "26", "fs": "64", "gs": "65", "ss": "36" }

def RegisterSegment(reg):
	if (reg in Reg_32) or (reg in Reg_16):
		if (reg == "esp") or (reg == "ebp") or (reg == "sp") or (reg == "bp"):
			return "ss"
		else:
			return "ds"
	return ""

class Lexem:
	text = ""
	index = 0
	type = ""
	pos = 0

class Sentence:
	printed = False
	source = ""
	prefix = ""
	bytes = ""
	offset = 0
	length = 0
	label = -1
	mnemo = -1
	operands = []

	def __init__(self, lex):
		self.source = lex[0]
		self.lexems = lex[1]
		self.operands = []
		i = 0
		if (i < len(self.lexems)) and (self.lexems[i].type == 'identifier'):
			self.label = i + 1
			i += 1
			if (i < len(self.lexems)) and (self.lexems[i].text == ":"):
				i += 1
			
		if (i < len(self.lexems)) and ((self.lexems[i].type == 'directive') or (self.lexems[i].type == 'command')):
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
				print("| %2d | %-11s | %i | %16s |"%(lexem.index, lexem.text, len(lexem.text), lexem.type))
			print()

	def printOffsets(self):
		if self.printed:
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

class Lexer:
	current_line = 0
	sentences = []
	segments = {}
	filename = ""
	segment = ""
	symbols = {}
	assume = {}
	eques = {}
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
					if lexem.text in Reg_32:
						lexem.type = dictionary[Reg_32]
					elif lexem.text in Reg_8:
						lexem.type = dictionary[Reg_8]
					elif lexem.text in SegmentReg:
						lexem.type = dictionary[SegmentReg]
					elif lexem.text in Reg_16:
						lexem.type = dictionary[Reg_16]
					elif lexem.text in Command:
						lexem.type = dictionary[Command]
					elif lexem.text in Directive:
						lexem.type = dictionary[Directive]
					elif lexem.text in TypeOperator:
						lexem.type = dictionary[TypeOperator]
					elif lexem.text in TypeDefinitor:
						lexem.type = dictionary[TypeDefinitor]
					else:
						lexem.type = 'identifier'
				elif line[i].isdigit():
					numb_er = False
					while (i < len(line)) and (line[i].isdigit() or (line[i].lower() in Hex)):
						lexem.text += line[i]
						i += 1
					lexem.text = lexem.text.lower()
					if (i < len(line)) and (line[i] == 'h'):
						i += 1
						for j in range (0, len(lexem.text)):
							if not (lexem.text[j].isdigit() or lexem.text[j] in Hex):
								numb_er = True
						if numb_er == True:
							print("Wrong number in line " + str(self.current_line))
							exit(1)

						lexem.type = 'Heximal'
					elif lexem.text[len(lexem.text) - 1] == "b":
						for j in range (0, len(lexem.text) - 1):
							if not lexem.text[j] in Binary:
								numb_er = True
						if numb_er == True:
							print("Wrong number in line " + str(self.current_line))
							exit(1)

						lexem.type = 'Binary'
						lexem.text = lexem.text[:-1]
					else:
						for j in range (0, len(lexem.text)):
							if not lexem.text[j].isdigit():
								numb_er=True

						if numb_er == True:
							print("Wrong number in line " + str(self.current_line))
							exit(1)

						lexem.type = 'Decimal'
				elif line[i] == '"':
					i += 1
					while (i < len(line)) and (line[i] != '"') and (line[i] != '\n'):
						lexem.text += line[i]
						i += 1

					if (i >= len(line)) or (line[i] != '"'):
						print("Wrong string in line " + str(self.current_line))
						exit(1)
					lexem.type = 'Text Constant'
					i += 1	
				elif line[i] in SingleSymbol:
					lexem.text = line[i]
					lexem.type = dictionary[SingleSymbol]
					i += 1
				else:
					print("Illegal symbol in line " + str(self.current_line) + " position " + str(i))
					print("Symbol: " + line[i])
					exit(1)

				if lexem.text in self.eques:
					replaced = line.replace(lexem.text, self.symbols[lexem.text].value)
					for lexem in self.eques[lexem.text]:
						lexem.index = index
						index += 1
						lexems.append(lexem)
				else:
					lexems.append(lexem)
		return [replaced, lexems]

	def equType(self, lexems):
		if len(lexems) == 1:
			type = lexems[0].type
			if (type == 'Binary') or (type == 'Decimal') or (type == 'Heximal'):
				return "NUMBER"

		if len(lexems) != 0:
			return "TEXT"
		else:
			return ""

	def parseREG(self, lexems, i):
		reg = None
		if i < len(lexems):
			if (lexems[i].text in Reg_32) or (lexems[i].text in Reg_16) or (lexems[i].text in Reg_8) or (lexems[i].text in SegmentReg):
				reg = lexems[i].text
			else:
				print("Illegal operand type on line " + str(self.current_line))
				exit(1)
			i += 1
		else:
			print("Missing operand on line " + str(self.current_line))
			exit(1)
		return [i, reg]

	def parseIMM(self, lexems, i):
		imm = None
		if i < len(lexems):
			if lexems[i].type == 'Binary':
				imm = int(lexems[i].text, 2)
			elif lexems[i].type == 'Decimal':
				imm = int(lexems[i].text, 10)
			elif lexems[i].type == 'Heximal':
				imm = int(lexems[i].text, 16)
			else:
				print(lexems[i].type)
				print("Illegal operand type on line " + str(self.current_line))
				exit(1)
			i += 1
		else:
			print("Missing operand on line " + str(self.current_line))
			exit(1)
		return [i, imm]

	def parseMEM(self, lexems, i):
		ptr = None
		seg = None
		reg = None
		disp = 0
		if i < len(lexems):
			if lexems[i].text in TypeDefinitor:
				ptr = lexems[i].text
				i += 1
				if (i < len(lexems)) and (lexems[i].text == "ptr"):
					i += 1
				else:
					print("Missing PTR operator on line " + str(self.current_line))
					exit(1)

			if i < len(lexems):
				if lexems[i].text in SegmentReg:
					seg = lexems[i].text
					i += 1
					if (i < len(lexems)) and (lexems[i].text == ":"):
						i += 1
					else:
						print("Missing ':' operator on line " + str(self.current_line))
						exit(1)
			else:
				print("Illegal operand on line " + str(self.current_line))
				exit(1)

			if (i < len(lexems)) and (lexems[i].text == "["):
				i += 1
				if (i < len(lexems)) and ((lexems[i].text in Reg_32) or (lexems[i].text in Reg_16)):
					reg = lexems[i].text
					i += 1
					if (i < len(lexems)) and (lexems[i].text == "+"):
						i += 1
						if (i < len(lexems)):
							if lexems[i].type == 'Binary':
								disp = int(lexems[i].text, 2)
							elif lexems[i].type == 'Decimal':
								disp = int(lexems[i].text, 10)
							elif lexems[i].type == 'Heximal':
								disp = int(lexems[i].text, 16)
							else:
								print("Expected operand disp on line " + str(self.current_line))
								exit(1)
							i += 1
							if (i < len(lexems)) and (lexems[i].text == "]"):
								i += 1
							else:
								print(lexems[i].text)
								print("Missing ']' operator on line " + str(self.current_line))
								exit(1)
						else:
							print("Missing operand disp on line" + str(self.current_line))
							exit(1)
					else:
						print("Missing '+' operator on line " + str(self.current_line))
						exit(1)
				else:
					print("Expected base register on line " + str(self.current_line))
					exit(1)
			else:
				print(lexems[i].text)
				print("Illegal operand type on line " + str(self.current_line))
				exit(1)
		else:
			print("Missing operand on line " + str(self.current_line))
			exit(1)
		return [i, ptr, seg, reg, disp]

	def immSize(self, ptr, imm):
		if ptr:
			if ptr == "byte":
				if imm in range(-256, 256):
					return 1
				else:
					print("Value out of range at line " + str(self.current_line))
			elif ptr == "word":
				if imm in range(-65536, 65536):
					if imm in range(-128, 128):
						return 1
					else:
						return 2

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
		return [i, operand1, operand2]

	def parseIdent(self, lexems, i):
		ident = None
		if i < len(lexems):
			if lexems[i].type == 'identifier':
				ident = lexems[i].text
			else:
				print(lexems[i].type)
				print("Illegal operand type on line " + str(self.current_line))
				exit(1)
			i += 1
		else:
			print("Missing operand on line " + str(self.current_line))
			exit(1)
		return [i, ident]

	def parse(self, path):
		self.filename = path.split(":")[0]
		for line in open(path, 'r'):
			sentence = Sentence(self.lex(line))
			lexems = sentence.lexems
			self.current_line += 1
			ident = None
			mnemo = None
			i = 0
			if (i < len(lexems)) and (lexems[i].type == 'identifier'):
				label = lexems[i]
				i += 1
				if (i < len(lexems)):
					if (lexems[i].text == ":"):
						i += 1
						sentence.printed = True
						symbol = Symbol()
						symbol.segment = self.segment
						symbol.value = "%.4X" % self.offset
						symbol.type = "L NEAR"
						self.symbols[label.text] = symbol
					else:
						ident = label
						
			if (i < len(lexems)) and ((lexems[i].type == 'directive') or (lexems[i].type == 'command')):
				mnemo = lexems[i]
				i += 1

			if mnemo:
				if ident:
					if mnemo.text == "segment":
						sentence.printed = True
						self.offset = 0
						self.segment = ident.text
					elif mnemo.text == "equ":
						equ = []
						while (i < len(lexems)):
							equ.append(lexems[i])
							i += 1
						self.eques[ident.text] = equ
						symbol = Symbol()
						symbol.type = self.equType(equ)
						if symbol.type == "NUMBER":
							if equ[0].type == 'Binary':
								sentence.prefix = " = %.4X" % int(equ[0].text, 2) 
							elif equ[0].type == 'Decimal':
								sentence.prefix = " = %.4X" % int(equ[0].text, 10) 
							else:
								sentence.prefix = " = %.4X" % int(equ[0].text, 16) 
						else:
							sentence.prefix = " =     "
							symbol.type = "TEXT"
							if len(equ):
								symbol.value = line[equ[0].pos:equ[len(equ) - 1].pos + 1]
						self.symbols[ident.text] = symbol
					elif mnemo.text == "db":
						sentence.printed = True
						sentence.length = 1

						symbol = Symbol()
						symbol.type = "L BYTE"
						symbol.value = "%.4X" % self.offset
						symbol.segment = self.segment

						if i < len(lexems):
							if (lexems[i].type == 'Text Constant'):
								sentence.length = len(lexems[i].text)
							i += 1
						else:
							print("Missing operand on line " + str(self.current_line))
							exit(1)

						self.symbols[ident.text] = symbol
					elif mnemo.text == "dw":
						sentence.printed = True
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
						sentence.printed = True
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
					elif mnemo.text == "ends":
						sentence.printed = True
						self.segments[ident.text] = self.offset
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
							if lexems[i].text in SegmentReg:
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

							if (i < len(lexems)) and (lexems[i].type == 'identifier'):
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
					elif mnemo.text in Command:
						sentence.printed = True
						instr = mnemo
						
						if instr.text == "cld":
							sentence.length = 1
						elif instr.text == "pop":
							operand = self.parseREG(lexems, i)
							i = operand[0]
							reg = operand[1]

							if reg in Reg_32:
								sentence.length = 2
							elif reg in Reg_16:
								sentence.length = 1
							elif reg in SegmentReg:
								sentence.length = 1
								if (reg == "fs") or (reg == "gs"):
									sentence.length += 1
								elif reg == "cs":
									print("Illegal use of CS register on line " + str(self.current_line))
									exit(1)
						elif instr.text == "mul":
							operand = self.parseMEM(lexems, i)
							i = operand[0]
							ptr = operand[1]

							if not ptr:
								print("Illegal operand size on line " + str(current_line))
								exit(1) 

							seg = operand[2]
							reg = operand[3]
							disp = operand[4] & 0xFFFFFFFF

							sentence.length = 2
							if reg in Reg_32:
								sentence.length += 1

							if ptr == "dword":
								sentence.length += 1

							if seg and (seg != RegisterSegment(reg)):
								sentence.length += 1

							if (reg == "esp") or (reg == "sp"):
								sentence.length += 1

							if disp:
								if disp in range(0x80, 0xFF80):
									sentence.length += 4
								elif disp in range(0x10000, 0xFFFF80):
									sentence.length += 4
								elif disp in range(0x1000000, 0xFFFFFF80):
									sentence.length += 4
								else:
									sentence.length += 1
						elif instr.text == "mov":
							operands = self.parseOperands(self.parseREG, self.parseREG, lexems, i)
							i = operands[0]

							sentence.length = 2

							if (operands[1][1] in Reg_32) and (operands[2][1] in Reg_32):
								sentence += 1

							if i < len(lexems):
								print("Extra characters on line " + str(self.current_line))
								exit(1)
						elif instr.text == "xor":
							operands = self.parseOperands(self.parseREG, self.parseMEM, lexems, i)
							i = operands[0]

							ptr = operands[2][1]
							seg = operands[2][2]
							reg = operands[2][3]
							disp = operands[2][4] & 0xFFFFFFFF

							sentence.length = 2

							if reg in Reg_32:
								sentence.length += 1

							if (ptr and (ptr == "dword")) or (operands[1][1] in Reg_32):
								sentence.length += 1

							if seg and (seg != RegisterSegment(reg)):
								sentence.length += 1


							if (reg == "esp") or (reg == "sp"):
								sentence.length += 1

							if disp:
								if disp in range(0x80, 0xFF80):
									sentence.length += 4
								elif disp in range(0x10000, 0xFFFF80):
									sentence.length += 4
								elif disp in range(0x1000000, 0xFFFFFF80):
									sentence.length += 4
								else:
									sentence.length += 1

							if i < len(lexems):
								print("Extra characters on line " + str(self.current_line))
								exit(1)
						elif instr.text == "sub":
							operands = self.parseOperands(self.parseMEM, self.parseREG, lexems, i)
							i = operands[0]

							ptr = operands[1][1]
							seg = operands[1][2]
							reg = operands[1][3]
							disp = operands[1][4] & 0xFFFFFFFF

							sentence.length = 2

							if reg in Reg_32:
								sentence.length += 1

							if (ptr and (ptr == "dword")) or (operands[2][1] in Reg_32):
								sentence.length += 1

							if seg and (seg != RegisterSegment(reg)):
								sentence.length += 1


							if (reg == "esp") or (reg == "sp"):
								sentence.length += 1

							if disp:
								if disp in range(0x80, 0xFF80):
									sentence.length += 4
								elif disp in range(0x10000, 0xFFFF80):
									sentence.length += 4
								elif disp in range(0x1000000, 0xFFFFFF80):
									sentence.length += 4
								else:
									sentence.length += 1

							if i < len(lexems):
								print(lexems)
								print("Extra characters on line " + str(self.current_line))
								exit(1)
						elif instr.text == "cmp":
							operands = self.parseOperands(self.parseREG, self.parseIMM, lexems, i)
							i = operands[0]
							reg = operands[1][1]
							imm = operands[2][1] & 0xFFFFFFFF

							sentence.length = 2

							if reg in Reg_8:
								if imm > 0xFF:
									print("Value out of range on line " + str(self.current_line))
									exit(1)
							elif reg in Reg_16:
								if imm > 0xFFFF:
									print("Value out of range on line " + str(self.current_line))
									exit(1)
								sentence.length += 1
							else:
								sentence.length += 2
								if imm > 0xFF:
									sentence.length += 2

							if i < len(lexems):
								print("Extra characters on line " + str(self.current_line))
								exit(1)
						elif instr.text == "add":
							operands = self.parseOperands(self.parseMEM, self.parseIMM, lexems, i)
							i = operands[0]

							ptr = operands[1][1]
							seg = operands[1][2]
							reg = operands[1][3]
							disp = operands[1][4] & 0xFFFFFFFF
							imm = operands[2][1] & 0xFFFFFFFF
							req = self.immSize(ptr, imm)

							sentence.length = 2

							if reg in Reg_32:
								sentence.length += 1

							if ptr and (ptr == "dword"):
								sentence.length += 1

							if seg and (seg != RegisterSegment(reg)):
								sentence.length += 1


							if (reg == "esp") or (reg == "sp"):
								sentence.length += 1


							if disp:
								if disp in range(0x80, 0xFF80):
									sentence.length += 4
								elif disp in range(0x10000, 0xFFFF80):
									sentence.length += 4
								elif disp in range(0x1000000, 0xFFFFFF80):
									sentence.length += 4
								else:
									sentence.length += 1

							if req == 1:
								sentence.length += 1
							elif req == 2:
								sentence.length += 2
							else:
								sentence.length += 4
						elif instr.text == "jc":
							operand = self.parseIdent(lexems, i)
							i = operand[0]
							sentence.length = 4
						elif instr.text == "jmp":
							operand = self.parseMEM(lexems, i)
							i = operand[0]
							ptr = operand[1]

							seg = operand[2]
							reg = operand[3]
							disp = operand[4] & 0xFFFFFFFF

							sentence.length = 2
							if reg in Reg_32:
								sentence.length += 1

							if seg and (seg != RegisterSegment(reg)):
								sentence.length += 1

							if (reg == "esp") or (reg == "sp"):
								sentence.length += 1

							if disp:
								if disp in range(0x80, 0xFF80):
									sentence.length += 4
								elif disp in range(0x10000, 0xFFFF80):
									sentence.length += 4
								elif disp in range(0x1000000, 0xFFFFFF80):
									sentence.length += 4
								else:
									sentence.length += 1
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
			print("%-32s\t%-7s\t%-.4X" % (segment, "16 Bit", self.segments[segment]))
		
		print("\n\nSymbols:\n                N a m e         	Type	Value	Attr\n");
		for symbol in sorted(self.symbols):
			print("%-32s\t%-7s\t%-s\t%s" % (symbol, self.symbols[symbol].type, self.symbols[symbol].value, self.symbols[symbol].segment))

		print("\n%-32s\tTEXT  %s" % ("@FILENAME", self.filename))

if __name__=="__main__":
	lexer = Lexer()
	lexer.parse("test.asm")
	lexer.printLexical()
	print()
	lexer.printListing()