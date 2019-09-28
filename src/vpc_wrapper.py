# practical problems did this and its really bad i did this in like february to add clang support

# first upgrade platform toolset, then fix up vcprojs 
# don't actually need to change the .sln

import sys
import os
import platform
import subprocess
import ctypes
import hashlib
from enum import IntEnum
from enum import Enum

class vpc_parse_item:
	def __init__(self, iItemName, iPItemName, iAssociatedDefines = []):
		self.itemName = iItemName
		self.pItemName = iPItemName
		
hDLL = 0 #handle to kernel32
hConsole = 0
verbosePrint = False
globalGroups = []
globalDefines = [] 
globalProjects = []
globalFiles = []
globalProjectFiles = []

class GENTYPE(IntEnum):
	VS2019 = 0,
	CLANG = 1
	
globalGenType = GENTYPE.VS2019

use_lld_linker = True
use_cpp_17 = False

PLATFORM_EXECUTABLE_EXTENSION = ""

class PLATFORM(IntEnum):
	ERR = 0,
	WINDOWS = 1,
	MACOS = 2,
	LINUX = 3
	
platformType = PLATFORM.ERR

system = platform.system()

if(system == "Windows"):
	globalDefines.append(vpc_parse_item("WIN32", "NULL"))
	platformType = PLATFORM.WINDOWS
	PLATFORM_EXECUTABLE_EXTENSION = ".exe"
elif(system=="MacOS"):
	globalDefines.append(vpc_parse_item("OSX", "NULL"))
	platformType = PLATFORM.MACOS
	PLATFORM_EXECUTABLE_EXTENSION = ""
elif(system=="Linux"):
	globalDefines.append(vpc_parse_item("POSIX", "NULL"))
	platformType = PLATFORM.LINUX
	PLATFORM_EXECUTABLE_EXTENSION = ""
else:
	print("vpc_wrapper could not detect your operating system! Expect errors!")

if(platformType == PLATFORM.WINDOWS):
	class VCOLORS(IntEnum):
		BLACK = 0	
		BLUE = 1
		GREEN = 2
		CYAN = 3
		RED = 4
		MAGENTA = 5
		BROWN = 6
		LIGHTGRAY = 7
		DARKGRAY = 8
		LIGHTBLUE = 9
		LIGHTGREEN = 10
		LIGHTCYAN = 11
		LIGHTRED = 12
		LIGHTMAGENTA = 13
		YELLOW = 14
		WHITE = 15
	hDLL = ctypes.cdll.LoadLibrary("Kernel32.dll")
	hConsole = hDLL.GetStdHandle(-11)
	def vprint(msg, color = VCOLORS.WHITE):
		global verbosePrint
		if(verbosePrint):
			global hDLL
			hDLL.SetConsoleTextAttribute(hConsole, color)
			print(msg)
			hDLL.SetConsoleTextAttribute(hConsole, VCOLORS.WHITE)
	def cprint(msg, color = VCOLORS.WHITE):
		global hDLL
		hDLL.SetConsoleTextAttribute(hConsole, color)
		print(msg)
		hDLL.SetConsoleTextAttribute(hConsole, VCOLORS.WHITE)
else:
	class VCOLORS(Enum):
		BLACK = "\033[31;40m"	
		BLUE = "\033[34;40m"
		GREEN = "\033[32;40m"
		CYAN = "\033[36;40m"
		RED = "\033[31;40m"
		MAGENTA = "\033[35;40m"
		BROWN = "\033[33;40m" # defined as "yellow" but its close to brown on some platforms
		LIGHTGRAY = "\033[37;40m"
		DARKGRAY = "\033[30;40m"
		LIGHTBLUE = "\033[34;1;40m"
		LIGHTGREEN = "\033[32;1;40m"
		LIGHTCYAN = "\033[36;1;40m"
		LIGHTRED = "\033[31;1;40m"
		LIGHTMAGENTA = "\033[35;1;40m"
		YELLOW = "\033[33;1;40m"
		WHITE = "\033[37;1;40m"
	#platform setup
	def vprint(msg, color = VCOLORS.WHITE):
		global verbosePrint
		if(verbosePrint):
			print(color + msg + "\033[38m")
	def cprint(msg, color = VCOLORS.WHITE):
		print(color + msg + "\033[38m")
	
ignoreOptions = [
	"/q",
	#"/v"
	"/f",
	"/dp",
	"/testmode",
	"/srcctl",
	"/nounity",
	"/h",
	"/?",
	"/platfoms",
	"/games",
	"/projects",
	"/groups",
	"/properties",
	#"/profile",
	#"/retail",
	#"/callcap",
	#"/fastcap",
	#"/nofpo",
	#"/lv",
	#"/demo",
	#"/no_steam",
	#"/qtdebug",
	#"/no_ceg",
	#"/upload_ceg",
	"/mksln",
	"/p4sln",
	"/nop4add",
	"/slnitems",
	"/showdeps",
	"/checkfiles",
]
versionOptions = [
	"/2013",
	"/2012",
	"/2010",
	"/2005"
]

class PARSE_OPERATION(IntEnum):
	#NO_OPERATION = 0
	START_OF_OPERATION = 1
	LOGICAL_AND = 2
	LOGICAL_OR = 3
	
class parseState(object):	
	def __init__(self, operation):
		self.poLastOperation = operation
		self.bValue = False
	
def parseTokenValidity(token):
	if(token[0]=="!"):
		tokenDefName = token[2:].upper()
	else:
		tokenDefName = token[1:].upper()
		#condObj.append(tokenDefName)
	bMatchFound = False
	for define in globalDefines:
		if define.itemName == tokenDefName:
			bMatchFound = True
			break
	
	if(token[0] != '!' and bMatchFound) or (token[0]=="!" and not bMatchFound):
		vprint("\t\t\t\tToken \""+token+"\" is satisfied by the defines.", VCOLORS.DARKGRAY)
		return True
	else:
		vprint("\t\t\t\tToken \""+token+"\" is NOT satisfied by the defines.", VCOLORS.YELLOW)
		return False
		
class checkConditionalResult:
	def __init__(self):
		self.defines = []
		self.result = False
def checkConditionals(conditionalsArray):
	
	#strip whitespace
	#conditionalObject = checkConditionalResult()
	
	while(conditionalsArray[0] == " " or conditionalsArray[0] == "\t"):
		conditionalsArray = conditionalsArray[1:]
	if(conditionalsArray.startswith("[") and conditionalsArray.endswith("]")):
		depth = 0
		index = 0
		resultsArr = [] # index in array = depth, boolean value for it
		spars = parseState(PARSE_OPERATION.START_OF_OPERATION)
		resultsArr.append(spars)
		
		controlCharacters = ['[', ']', '(', ')', "&&", "||"]
		seekIndex = 0
		
		splitArray = []
		while(len(conditionalsArray)):
			if conditionalsArray[seekIndex] == ' ':
				if(seekIndex!=0):
					splitArray.append(conditionalsArray[:seekIndex])
				conditionalsArray = conditionalsArray[seekIndex+1:]
				seekIndex = -1
			else:
				for charset in controlCharacters:
					if conditionalsArray[seekIndex:seekIndex+len(charset)] == charset:
						if(seekIndex!=0):
							splitArray.append(conditionalsArray[:seekIndex])
						splitArray.append(conditionalsArray[seekIndex:seekIndex+len(charset)])
						conditionalsArray = conditionalsArray[seekIndex+len(charset):]
						seekIndex = -1
						if(len(conditionalsArray) == 0):
							break
			seekIndex+=1
		#print(str(splitArray))
		while(depth > -1):
		#while(0):
			#print("Index:" + str(index))
			curValue = splitArray[index]
			#print(curValue)
			if(curValue == '['):
				pass
			elif(curValue == "("):
				depth+=1
				pars = parseState(PARSE_OPERATION.START_OF_OPERATION)
				resultsArr.append(pars)
			elif(curValue == ")"):
				depth-=1
				#complete the operation
				if(resultsArr[depth].poLastOperation == PARSE_OPERATION.START_OF_OPERATION):
					resultsArr[depth].bValue = resultsArr[depth+1]
				elif(resultsArr[depth].poLastOperation == PARSE_OPERATION.LOGICAL_AND):
					resultsArr[depth].bValue = resultsArr[depth].bValue and resultsArr[depth+1].bValue
				elif(resultsArr[depth].poLastOperation == PARSE_OPERATION.LOGICAL_OR):
					resultsArr[depth].bValue = resultsArr[depth].bValue or resultsArr[depth+1].bValue
				
			elif(curValue == "&&"):
				resultsArr[depth].poLastOperation = PARSE_OPERATION.LOGICAL_AND
			elif(curValue == "||"):
				resultsArr[depth].poLastOperation = PARSE_OPERATION.LOGICAL_OR
			elif(curValue == ']'):
				if(depth == 0): 
					depth = -1
				else:
					cprint("Malformatted conditional statement - NOT appending", VCOLORS.RED)
					return False
			else:
				#we have a condition!
				if(resultsArr[depth].poLastOperation == PARSE_OPERATION.START_OF_OPERATION):
					resultsArr[depth].bValue = parseTokenValidity(curValue)
					#lvalue = PARSE_OPERATION.NO_OPERATION
				elif(resultsArr[depth].poLastOperation == PARSE_OPERATION.LOGICAL_AND):
					resultsArr[depth].bValue = resultsArr[depth].bValue and parseTokenValidity(curValue)
				elif(resultsArr[depth].poLastOperation == PARSE_OPERATION.LOGICAL_OR):
					resultsArr[depth].bValue = resultsArr[depth].bValue or parseTokenValidity(curValue)
					
			index += 1
						
		if(resultsArr[0].bValue):
			vprint("\t\tConditionals satisfied - appending", VCOLORS.LIGHTGRAY)
			return True
		else:
			vprint("\t\tConditionals unsatisfied - NOT appending", VCOLORS.RED)
	else:
		cprint("Missing brackets on conditional statement - NOT appending", VCOLORS.RED)
		vprint("Starts with " + conditionalsArray[0] + ", ends with " + conditionalsArray[-1])
	return False
def VPC_items_to_list(file, searchList, list, markToken):
	#print("searchList = " + str(searchList))
	vpcFile = open(file, "r")
	print("\nScanning VGC files... [" + file + "]")
	#parse our groups
	line = vpcFile.readline()
	while(line):
		if line.startswith(markToken):
			entryName = line[len(markToken) + 1:].rstrip().replace('"', '')
			bMatched = False
			for listIndex in range(0, len(searchList)):
				if searchList[listIndex].itemName == entryName:
					bMatched = True
					break
			if bMatched:
				#vprint("~~~ MATCHED ~~~")
				vprint("Found VGC group entry \"" + entryName + "\"", VCOLORS.LIGHTGRAY)
				line = vpcFile.readline()
				while(line.find("}")==-1):
					if(line.rstrip()!="{"):
						while(line[0]=='\t' or line[0]==' '):
							line = line[1:] #strip off starting whitespace
						if(line.startswith("//")):
							line = vpcFile.readline()
							continue
						lineSplitPre = line.rstrip().split('\t')
						lineSplitRaw = []
						for item in lineSplitPre:
							itemSplit = item.split(' ')
							for subitem in itemSplit:
								lineSplitRaw.append(subitem)
						lineSplitReal = []
						lineTmpString = ""
						#vprint(str(lineSplitRaw))
						for item in lineSplitRaw:
							if(lineTmpString!=""):
								if(item.endswith('"')):
									lineSplitReal.append(lineTmpString + item[:-1])
									lineTmpString = ""
								else:
									lineTmpString += item
							elif(item.startswith('"')):
								if(item.endswith('"')):
									lineSplitReal.append(item.replace('"', ''))
								else:
									lineTmpString = item[1:]
							else:
								lineSplitReal.append(item)
						#argument 1 is the actual thing, everything else is part of the conditionals 
						if(len(lineSplitReal) > 1 and not lineSplitReal[1].startswith("//")):
							conditionalsStr = ' '.join(lineSplitReal[1:])
							vprint("\tFound item entry \"" + lineSplitReal[0] + "\", conditionals: " + conditionalsStr, VCOLORS.LIGHTGRAY)
							if(checkConditionals(conditionalsStr)):
								list.append(vpc_parse_item(lineSplitReal[0], entryName))
						else:
							vprint("\tFound item entry \"" + lineSplitReal[0] + "\", no conditionals - appending", VCOLORS.LIGHTGRAY)
							list.append(vpc_parse_item(lineSplitReal[0], entryName))
					line = vpcFile.readline()
		line = vpcFile.readline()
	vpcFile.close()
	
def generate_hash(filehash):
	f1md5 = hashlib.md5()
	with open(filehash, 'rb') as f1f:
		while True:
			data = f1f.read(65536)
			if not data:
				break
			f1md5.update(data)
		f1f.close()
	return f1md5.hexdigest()
def cmp_hash(f1, hashfile):
	hff = open(hashfile, 'r')
	hfh = hff.read()
	hff.close()
	filehash = generate_hash(f1)
	vprint("\t\tvpc_wrapper_LOCK: calculated hash is " + filehash + " ; saved hash is " + hfh)
	return filehash == hfh
def get_label(line):
	labelPos = line.find("Label=\"")
	labelStr = ""
	if(labelPos!=-1):
		try:
			while line[labelPos+7] != '"':
				labelStr+=line[labelPos+7]
				labelPos+=1
			return labelStr
		except IndexError:
			return labelStr
	else:
		return ""
def vpc_wrapper_fixup():
	if platformType == PLATFORM.WINDOWS:
		print("\nFixing Visual Studio projects...")
		
		for vcxproj in globalProjectFiles:
			if not os.path.exists(vcxproj):
				cprint("File \"" + vcxproj + "\" missing, vpc_wrapper failed.", VCOLORS.RED)
				return
			
			# open the file 
			
			
			if(os.path.exists(vcxproj + ".vpc_wrapper_lock")):
				if cmp_hash(vcxproj, vcxproj + ".vpc_wrapper_lock"):
					vprint("\tSkipping reparse of file \"" + vcxproj + "\", marked as already parsed. If you want to regenerate this, use a full rebuild.", VCOLORS.DARKGRAY)
					continue
				else:
					vprint("\tWarning: Hash invalid, re-parsing file.", VCOLORS.YELLOW)
					os.remove(vcxproj + ".clang_vpc_lock")
			
			vcfile = open(vcxproj, "r")
			vcfile = open(vcxproj, "r")
			tmpfile = open(vcxproj + ".tmp", "w")
			curline = vcfile.readline() # second line
			
			in_clcompile = False
			in_itemdefinitiongroup = False
			found_language_standard = False
			in_config = False
			apply_nolld_at_next_endproperty = False
			while(curline):
				if(curline.find("<ClCompile>")!=-1 and in_itemdefinitiongroup):
					in_clcompile = True
					tmpfile.write(curline)
				elif(curline.find("</ClCompile>") != -1 and in_itemdefinitiongroup):
					if not found_language_standard and use_cpp_17:
						# no language standard found and cpp17 needed, then append it!
						tmpfile.write("\t  <LanguageStandard>stdcpp17</LanguageStandard>\n")
					in_clcompile = False
					tmpfile.write(curline)
				elif(curline.find("<ItemDefinitionGroup") != -1):
					in_itemdefinitiongroup = True
					tmpfile.write(curline)
				elif(curline.find("</ItemDefinitionGroup>") != -1):
					in_itemdefinitiongroup = False
					tmpfile.write(curline)
				elif(get_label(curline) == "Configuration"):
					in_config = True
					tmpfile.write(curline)
				elif(get_label(curline) == "UserMacros") and not use_lld_linker:
					apply_nolld_at_next_endproperty = (globalGenType == GENTYPE.CLANG)
					tmpfile.write(curline)
				elif(curline.find("</PropertyGroup>") != -1):
					in_config = False
					tmpfile.write(curline)
					if apply_nolld_at_next_endproperty:
						tmpfile.write("\t<PropertyGroup Label=\"LLVM\" Condition=\"'$(Configuration)|$(Platform)'=='Debug|Win32'\">\n\t  <UseLldLink>false</UseLldLink>\n\t</PropertyGroup>\n")
						tmpfile.write("\t<PropertyGroup Label=\"LLVM\" Condition=\"'$(Configuration)|$(Platform)'=='Release|Win32'\">\n\t  <UseLldLink>false</UseLldLink>\n\t</PropertyGroup>\n")
				elif in_clcompile:
					languageStandardStart = curline.find("<LanguageStandard>")
					languageStandardEnd = curline.find("</LanguageStandard>")
					if use_cpp_17 and languageStandardStart!=-1 and languageStandardEnd!=-1:
						tmpfile.write(curline[:languageStandardStart+18] + "stdcpp17" + curline[languageStandardEnd:])
						found_language_standard = True
					else:
						tmpfile.write(curline)
				elif in_config:
					platformToolset = curline.find("<PlatformToolset>")
					platformToolsetEnd = curline.find("</PlatformToolset>")
					if platformToolset != -1 and platformToolsetEnd != -1:
						if(globalGenType == GENTYPE.VS2019):
							tmpfile.write(curline[:platformToolset+17] + "v142" + curline[platformToolsetEnd:])
						elif(globalGenType == GENTYPE.CLANG):
							tmpfile.write(curline[:platformToolset+17] + "llvm" + curline[platformToolsetEnd:])
					else:
						tmpfile.write(curline)
				else:
					tmpfile.write(curline)
				curline = vcfile.readline()
			vcfile.close()
			tmpfile.close()
			os.remove(vcxproj)
			os.rename(vcxproj + ".tmp", vcxproj)
			
			# lock time
			hashlockf = open(vcxproj + ".vpc_wrapper_lock", 'w')
			hashlockf.write(generate_hash(vcxproj))
			hashlockf.close()
			
	else:
		# Mac and Linux code goes here! How does xcode work?!?
		pass
		
def vRemoveFile(str):
	vprint("Removing file \"" + str + "\"...", VCOLORS.DARKGRAY)
	try:
		os.remove(str)
	except OSError:
		cprint("Warning: Specified file \"" + str + "\" does not exist. Continuing.", VCOLORS.YELLOW)
def vpc_wrapper_cleanprojects(rall):
	for vcxproj in globalProjectFiles:
		vRemoveFile(vcxproj)
		vRemoveFile(vcxproj + ".filters")
		vRemoveFile(vcxproj + ".vpc_crc")
		vRemoveFile(vcxproj + ".vpc_wrapper_lock")
		if rall:
			os.remove(vcxproj + ".user") # we want to keep this
			
def checkForGameExtension(gameFile):
	if not os.path.exists(gameFile):
		cprint("File \"" + gameFile + "\" missing, vpc_wrapper failed.", VCOLORS.RED)
		return ""
	gameFile = open(gameFile, 'r')
	curLine = gameFile.readline()
	while(curLine):
		if(curLine.startswith("$Macro GAMENAME")):
			lsplit = curLine.replace("\t", " ").rstrip().split(" ")
			gameName = ""
			for substr in range(0, len(lsplit)):
				startsQuote = lsplit[substr].startswith('"')
				endsQuote = lsplit[substr].endswith('"')
				if substr == 0 or substr == 1:
					continue
				elif startsQuote and endsQuote:
					gameName += lsplit[substr][1:-1]
					break
				elif startsQuote:
					gameName += lsplit[substr][1:]
				elif endsQuote:
					gameName += lsplit[substr][:-1]
					break
				else:
					gameName += lsplit[substr]
			conditionalStart = curLine.find("[") #shitty hack
			conditionalEnd = curLine.find("]") # still shiii tyyy
			if(conditionalStart == -1 or conditionalEnd == -1):
				vprint("Couldn't find GAMENAME conditional... might be an issue?", VCOLORS.YELLOW)
				return gameName
			else:
				tokens = curLine[conditionalStart:conditionalEnd+1]
				if(checkConditionals(tokens)):
					return gameName
		curLine = gameFile.readline()
	return ""
def main():
	global globalGroups
	global globalDefines
	#global globalGame # i hate this so much
	global verbosePrint
	global globalGenType
	global use_lld_linker
	global use_cpp_17
	runVPC = True #for debugging
	cleanVPC = False
	cleanVPCRemoveAll = False
	fullRebuild = False
	argvcpy = sys.argv[1:]
	in_projSuffix = False
	in_mirrormode = False
	projSuffix = ""
	mirrordir = ""
	cprint("VPC Wrapper - Generates for LLVM and Visual Studio 2019", VCOLORS.GREEN)
	for arg in argvcpy:
		if in_projSuffix:
			projSuffix = arg
			in_projSuffix = False
		elif in_mirrormode:
			mirrordir = arg
			in_mirrormode = False
		else:
			if arg in ignoreOptions: 
				#vprint("Ignored option "+arg) 
				continue
			elif arg in versionOptions: 
				#vprint("Setting version option \"" +arg +"\" to \"" + versionOptions[0] + "\"")
				del argvcpy[argvcpy.index(arg)] #always use 2015
			elif arg == "/vs2019":
				globalGenType = GENTYPE.VS2019
				globalDefines.append(vpc_parse_item("VS2019", "NULL"))
			elif arg == "/clang":
				globalGenType = GENTYPE.CLANG
				globalDefines.append(vpc_parse_item("CLANG", "NULL"))
			elif arg == "/cpp17":
				use_cpp_17 = True
			elif arg == "/no_lld":
				use_lld_linker = False
			elif arg == "/v": 
				verbosePrint = True
				#vprint("Verbose printing ENABLED")
			elif arg == "/nrvpc":
				vprint("\"/nrvpc\" detected, not running VPC.", VCOLORS.DARKGRAY)
				runVPC = False
			elif arg == "/cleanvpc":
				cleanVPC = True
			elif arg == "/cleanvpc_all":
				cleanVPC = True
				cleanVPCRemoveAll = True
			elif arg == "/full_rebuild":
				cleanVPC = True
				fullRebuild = True
			elif arg == "/projsuffix":
				in_projSuffix = True
			elif arg == "/mirror":
				in_mirrormode = True
			elif arg == "/no_qt":
				try:
					allowqtind = globalDefines.find(vpc_parse_item("ALLOW_QT", "NULL"))
					del globalDefines[allowqtind] # pop out ALLOW_QT
				except ValueError:
					pass
			elif arg == "/no_schema":
				try:
					allowschemaind = globalDefines.index(vpc_parse_item("ALLOW_SCHEMA", "NULL"))
					del globalDefines[allowschemaind] # pop out ALLOW_SCHEMA
				except ValueError:
					pass
			elif arg == "/unity" or arg == "/forceunity":
				globalDefines.append(vpc_parse_item("ALLOW_UNITY", "NULL"))
			elif arg == "/allgames":
				vprint("\"/allgames\" detected, scanning default.vgc...", VCOLORS.DARKGRAY)
				VPC_items_to_list("vpc_scripts/default.vgc", '', globalDefines, "$Games")
			elif arg == "/noallgames":
				vprint("\"/noallgames\" detected, scanning default.vgc...", VCOLORS.DARKGRAY)
				tmp_def_list = []
				VPC_items_to_list("vpc_scripts/default.vgc", '', tmp_def_list, "$Games")
				for item in tmp_def_list:
					try:
						itemMasterPos = globalDefines.index(item)
						del globalDefines[itemMasterPos]
					except ValueError:
						pass
			elif arg.startswith("+"):
				globalGroups.append(vpc_parse_item(arg[1:], "NULL"))
			elif arg.startswith("/define:"):
				globalDefines.append(vpc_parse_item(arg[8:].upper(), "NULL"))
			elif arg.startswith("/"):
				globalDefines.append(vpc_parse_item(arg[1:].upper(), "NULL"))
				#globalGame = arg[1:].upper()
	vprint("\nGroups: ", VCOLORS.YELLOW)
	for g in globalGroups: vprint("\t" + g.itemName, VCOLORS.LIGHTGRAY)
	vprint("\nGames/Defines: ", VCOLORS.YELLOW)
	for d in globalDefines: vprint("\t" + d.itemName, VCOLORS.LIGHTGRAY)
	if(runVPC and not cleanVPC):
		print("\nRunning VPC...")
		cmdBuild =  os.path.join( os.path.dirname(os.path.abspath(__file__)), "devtools", "bin", "vpc" + PLATFORM_EXECUTABLE_EXTENSION) + " " + ' '.join(argvcpy) + " " + versionOptions[0] #append latest version
		if(globalGenType == GENTYPE.VS2019):
			cmdBuild += " /define:VS2019"
		elif(globalGenType == GENTYPE.CLANG):
			cmdBuild += " /define:CLANG"
		vprint("\tVPC Command: " + cmdBuild, VCOLORS.DARKGRAY)
		res = subprocess.call(cmdBuild)
		if(res!= 0):
			cprint("\n--- VPC failed, aborting script", VCOLORS.RED)
			sys.exit(-1)
			return -1
	else:
		print("\nSkipping VPC...")
	VPC_items_to_list("vpc_scripts/groups.vgc", globalGroups, globalProjects, "$Group")
	#VPC_items_to_list("vpc_scripts/groups.vgc", globalGroups, globalFiles, "$Group")
	VPC_items_to_list("vpc_scripts/projects.vgc", globalProjects, globalFiles, "$Project")
	#convert to vcproj filenames.
	#hack time because my name jeff
	
	for ind in range(0, len(globalFiles)):
		if platformType == PLATFORM.WINDOWS:
			
			firstVPCPath = os.path.join(os.path.dirname(os.path.abspath(__file__)), *(globalFiles[ind].itemName.split('/')))
			vcxprojDirectory = os.path.dirname(firstVPCPath)

			compositeFileName = vcxprojDirectory + "\\" + globalFiles[ind].pItemName
			gameExtension = checkForGameExtension(firstVPCPath)
			if(gameExtension!=""):
				compositeFileName += ("_" + gameExtension)
			if(projSuffix!=""):
				compositeFileName += ("_" + projSuffix)
			compositeFileName += ".vcxproj"
			if compositeFileName not in globalProjectFiles:
				vprint("\tComposite VCXPROJ Filename: \"" + compositeFileName + "\"", VCOLORS.DARKGRAY)
				globalProjectFiles.append(compositeFileName)	
		else:
			# Mac and Linux code goes here! How does xcode work?!?
			pass
	if cleanVPC:
		print("Removing all specified VCXPROJs and associated metadata...")
		vpc_wrapper_cleanprojects(cleanVPCRemoveAll)
	else:
		vpc_wrapper_fixup()
		
	if fullRebuild:
		# call the script again, but without /full_rebuild, /cleanvpc, and /cleanvpc_all
		cmdBuild = "python \"" + os.path.abspath(__file__) + "\" " + (" " + ' '.join(sys.argv[1:]).replace("/full_rebuild", '').replace("/cleanvpc", '').replace("/cleanvpc_all", ''))
		vprint("\tRecalling script with command: " + cmdBuild, VCOLORS.DARKGRAY)
		#subprocess.call(cmdBuild)
		#Run a few passes just to make sure we got everything. AKA VPC often crashes.
		failedAccum = 0
		while(failedAccum < 2):
			if(subprocess.call(cmdBuild)!=0):
				failedAccum += 1
			else:
				break

if __name__ == "__main__":
	main()