#!/usr/bin/python3

from enum import Enum
import sqlite3 as lite
import sys
import difflib
import shutil
import os
import platform
import subprocess
import fileinput
from distutils.dir_util import copy_tree
import settime

class Os(Enum):
  Windows = 1
  Linux = 2
  Unsupported = 10

passed = 0
failed = 0
workDir = 'work'
outDir = 'out'
errDir= 'errors'
dbDir = 'db'


def areFilesIdentical(filename1, filename2):
    with open(filename1, "rtU") as a:
        with open(filename2, "rtU") as b:
            return all(lineA == lineB for lineA, lineB in zip(a.readlines(), b.readlines()))

def replaceAll(file,searchExp,replaceExp):
    for line in fileinput.input(file, inplace=1):
        if searchExp in line:
            line = line.replace(" -std=c++11", "")
            line = line.replace(searchExp,replaceExp)
        sys.stdout.write(line)

def compareToRef(createdFile, referenceFile, scenario):
    #Windows 
    replaceAll(createdFile, os.path.abspath('work').replace('\\', '/'),'work')
    
    if not os.path.exists(referenceFile):
        print('Reference file does not exists')
        return
        
    diff = areFilesIdentical(referenceFile,createdFile)
    if not diff:
        global failed 
        failed += 1
        print('[ ' + scenario + ' ] FAILED!')
        with open(os.path.join(errDir, 'error_' + scenario + '.diff'), 'w+') as f:
            fromLines = open(createdFile, 'U').readlines()
            toLines = open(referenceFile, 'U').readlines()
            diff = difflib.unified_diff(fromLines, toLines, createdFile, referenceFile)
            f.writelines(diff)
    else:
        global passed 
        passed += 1
        print('[ ' + scenario + ' ] Succeeded!') 
  
def runBackportTool(path):
    curros = detectOs()
    if not os.path.exists(path):
        return False
        
    appName = "backport.exe" if curros == Os.Windows else "backport"
    if not os.path.exists(os.path.join(path, appName)):
        raise RuntimeError("Couldn't found backport tool")
    tool = os.path.join(path, appName)
    subprocess.call([tool, '-p', os.path.abspath(workDir), '-include='+os.path.abspath(workDir), '-dbfile='+os.path.join(os.path.abspath(dbDir), 'backport.db') ],)# stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=True)

def detectOs():
    if platform.system() == 'Windows':
        return Os.Windows
    elif platform.system() == 'Linux':
        return Os.Linux
    else:
        return Os.Unsupported
        
def runScenario(case, compUnit):
    copy_tree(os.path.join('test', case), workDir)

    shutil.copy(os.path.join('out', compUnit+'_compile_commands.json'), os.path.abspath(os.path.join('work', 'compile_commands.json')))
    runBackportTool('.')

    dbFullPath = os.path.join(os.path.abspath(dbDir), 'backport.db')
    outDirWithoutFile = os.path.join(os.path.abspath(outDir), case)
    outFullPath = os.path.join(outDirWithoutFile, compUnit+'_sql.sql')
    refFullPath = os.path.join('reference', case, compUnit + '_sql.ref')

    if not os.path.exists(outDirWithoutFile):
        os.mkdir(outDirWithoutFile)
        
    file = open(outFullPath, 'wt')
    with lite.connect(dbFullPath) as con:
        cur = con.cursor()
        data = "\n".join(con.iterdump())
        with file:
            file.write(data)
        compareToRef(outFullPath, refFullPath, case+'_'+compUnit)

def makeCompileCommands():
    ccFiles = ['a', 'c', 'other']
    for compile_command_file in ccFiles:
        f = open(os.path.join(os.path.abspath('compile_commands'), compile_command_file+'_template.json'), "r")
        copy = open(os.path.join('out', compile_command_file+'_compile_commands.json'), 'w')
        for line in f:
            copy.write(line.replace("{workDir}", os.path.abspath(workDir)).replace('\\', '/'))
        f.close()
        copy.close()
    

def init():
    settime.setTimesForFiles()
    dirs = [workDir, outDir, dbDir, errDir]
    for current_dir in dirs:
        if os.path.exists(current_dir):
            shutil.rmtree(current_dir)
        os.mkdir(current_dir)
    makeCompileCommands()

def main(): 
    init()
    runScenario('case1', 'a')
    runScenario('case1', 'c')
    runScenario('case2', 'a')
    runScenario('case2', 'c')
    runScenario('case3', 'a')
    runScenario('case3', 'c')
    runScenario('case4', 'a')
    runScenario('case4', 'c')
    runScenario('case5', 'c')
    runScenario('case5', 'a')
    runScenario('case6', 'a')
    runScenario('case6', 'c')
    runScenario('case7', 'a')
    runScenario('case7', 'c')
    runScenario('case8', 'a')
    runScenario('case8', 'c')
    runScenario('case9', 'a')
    runScenario('case9', 'c')
    runScenario('case10', 'a')
    runScenario('case10', 'c')
    runScenario('case11', 'a')
    runScenario('case11', 'c')
    runScenario('case12', 'a')
    runScenario('case12', 'c')
    runScenario('case13', 'a')
    runScenario('case13', 'c')
    runScenario('case14', 'a')
    runScenario('case14', 'c')
    runScenario('case15', 'a')
    runScenario('case15', 'c')
    runScenario('case16', 'a')
    runScenario('case16', 'c')
    runScenario('case17', 'a')
    runScenario('case17', 'c')
    runScenario('case17', 'other')

    print('--------------------')
    print('Total: '+ repr(passed+failed) + ' \t passed: ' + repr(passed) + ' failed: \t' +  repr(failed))

if __name__ == '__main__':
    main()