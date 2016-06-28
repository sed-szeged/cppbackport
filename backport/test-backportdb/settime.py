#!/usr/bin/python3

import os

def setTimesForFiles():
    # test\case1 files
    os.utime(os.path.join('test', 'case1', 'A.cpp'), (1427387312, 1427387312))
    os.utime(os.path.join('test', 'case1', 'A.h'), (1427218226, 1427218226))
    os.utime(os.path.join('test', 'case1', 'All.h'), (1427389099, 1427389099))
    os.utime(os.path.join('test', 'case1', 'B.cpp'), (1427215320, 1427215320))
    os.utime(os.path.join('test', 'case1', 'B.h'), (1427218446, 1427218446))
    os.utime(os.path.join('test', 'case1', 'C.cpp'), (1427387371, 1427387371))
    os.utime(os.path.join('test', 'case1', 'C.h'), (1427215817, 1427215817))
    os.utime(os.path.join('test', 'case1', 'IncAll.h'), (1427220376, 1427220376))
    os.utime(os.path.join('test', 'case1', 'OnlyA.h'), (1427215660, 1427215660))
    os.utime(os.path.join('test', 'case1', 'OnlyB.h'), (1427214610, 1427214610))
    os.utime(os.path.join('test', 'case1', 'OnlyC.h'), (1427212284, 1427212284))

    # test\case10 files
    os.utime(os.path.join('test', 'case10', 'A.cpp'), (1427387312, 1427387312))
    os.utime(os.path.join('test', 'case10', 'A.h'), (1427813252, 1427813252))
    os.utime(os.path.join('test', 'case10', 'All.h'), (1427812159, 1427812159))
    os.utime(os.path.join('test', 'case10', 'B.cpp'), (1427215320, 1427215320))
    os.utime(os.path.join('test', 'case10', 'B.h'), (1427218446, 1427218446))
    os.utime(os.path.join('test', 'case10', 'C.cpp'), (1427461578, 1427461578))
    os.utime(os.path.join('test', 'case10', 'C.h'), (1427813274, 1427813274))
    os.utime(os.path.join('test', 'case10', 'IncAll.h'), (1427220376, 1427220376))
    os.utime(os.path.join('test', 'case10', 'OnlyB.h'), (1427214610, 1427214610))

    # test\case11 files
    os.utime(os.path.join('test', 'case11', 'A.cpp'), (1427387312, 1427387312))
    os.utime(os.path.join('test', 'case11', 'A.h'), (1427813252, 1427813252))
    os.utime(os.path.join('test', 'case11', 'All.h'), (1427899693, 1427899693))
    os.utime(os.path.join('test', 'case11', 'B.cpp'), (1427215320, 1427215320))
    os.utime(os.path.join('test', 'case11', 'B.h'), (1427218446, 1427218446))
    os.utime(os.path.join('test', 'case11', 'C.cpp'), (1427461578, 1427461578))
    os.utime(os.path.join('test', 'case11', 'C.h'), (1427813274, 1427813274))
    os.utime(os.path.join('test', 'case11', 'OnlyB.h'), (1427214610, 1427214610))

    # test\case12 files
    os.utime(os.path.join('test', 'case12', 'A.cpp'), (1427387312, 1427387312))
    os.utime(os.path.join('test', 'case12', 'A.h'), (1427813252, 1427813252))
    os.utime(os.path.join('test', 'case12', 'All.h'), (1427981730, 1427981730))
    os.utime(os.path.join('test', 'case12', 'AllNew.h'), (1427981210, 1427981210))
    os.utime(os.path.join('test', 'case12', 'B.cpp'), (1427215320, 1427215320))
    os.utime(os.path.join('test', 'case12', 'B.h'), (1427218446, 1427218446))
    os.utime(os.path.join('test', 'case12', 'C.cpp'), (1427461578, 1427461578))
    os.utime(os.path.join('test', 'case12', 'C.h'), (1427813274, 1427813274))
    os.utime(os.path.join('test', 'case12', 'OnlyB.h'), (1427214610, 1427214610))

    # test\case13 files
    os.utime(os.path.join('test', 'case13', 'A.cpp'), (1427387312, 1427387312))
    os.utime(os.path.join('test', 'case13', 'A.h'), (1427813252, 1427813252))
    os.utime(os.path.join('test', 'case13', 'All.h'), (1427982149, 1427982149))
    os.utime(os.path.join('test', 'case13', 'B.cpp'), (1427215320, 1427215320))
    os.utime(os.path.join('test', 'case13', 'B.h'), (1427218446, 1427218446))
    os.utime(os.path.join('test', 'case13', 'C.cpp'), (1427461578, 1427461578))
    os.utime(os.path.join('test', 'case13', 'C.h'), (1427813274, 1427813274))
    os.utime(os.path.join('test', 'case13', 'OnlyB.h'), (1427214610, 1427214610))

    # test\case13\newDir files
    os.utime(os.path.join('test', 'case13', 'newDir', 'AllNew.h'), (1427981210, 1427981210))

    # test\case14 files
    os.utime(os.path.join('test', 'case14', 'A.cpp'), (1427387312, 1427387312))
    os.utime(os.path.join('test', 'case14', 'A.h'), (1427813252, 1427813252))
    os.utime(os.path.join('test', 'case14', 'All.h'), (1427982149, 1427982149))
    os.utime(os.path.join('test', 'case14', 'B.cpp'), (1427215320, 1427215320))
    os.utime(os.path.join('test', 'case14', 'B.h'), (1427218446, 1427218446))
    os.utime(os.path.join('test', 'case14', 'C.cpp'), (1427461578, 1427461578))
    os.utime(os.path.join('test', 'case14', 'C.h'), (1427813274, 1427813274))
    os.utime(os.path.join('test', 'case14', 'OnlyB.h'), (1427214610, 1427214610))

    # test\case14\newDir files
    os.utime(os.path.join('test', 'case14', 'newDir', 'AllNew.h'), (1427985834, 1427985834))

    # test\case15 files
    os.utime(os.path.join('test', 'case15', 'A.cpp'), (1427387312, 1427387312))
    os.utime(os.path.join('test', 'case15', 'A.h'), (1427813252, 1427813252))
    os.utime(os.path.join('test', 'case15', 'All.h'), (1427985886, 1427985886))
    os.utime(os.path.join('test', 'case15', 'B.cpp'), (1427215320, 1427215320))
    os.utime(os.path.join('test', 'case15', 'B.h'), (1427218446, 1427218446))
    os.utime(os.path.join('test', 'case15', 'C.cpp'), (1427461578, 1427461578))
    os.utime(os.path.join('test', 'case15', 'C.h'), (1427813274, 1427813274))
    os.utime(os.path.join('test', 'case15', 'OnlyB.h'), (1427214610, 1427214610))

    # test\case16 files
    os.utime(os.path.join('test', 'case16', 'A.cpp'), (1427387312, 1427387312))
    os.utime(os.path.join('test', 'case16', 'A.h'), (1427987227, 1427987227))
    os.utime(os.path.join('test', 'case16', 'All.h'), (1427985886, 1427985886))
    os.utime(os.path.join('test', 'case16', 'ANew.h'), (1427987268, 1427987268))
    os.utime(os.path.join('test', 'case16', 'B.cpp'), (1427215320, 1427215320))
    os.utime(os.path.join('test', 'case16', 'B.h'), (1427218446, 1427218446))
    os.utime(os.path.join('test', 'case16', 'C.cpp'), (1427461578, 1427461578))
    os.utime(os.path.join('test', 'case16', 'C.h'), (1427813274, 1427813274))
    os.utime(os.path.join('test', 'case16', 'OnlyB.h'), (1427214610, 1427214610))

    # test\case17 files
    os.utime(os.path.join('test', 'case17', 'A.cpp'), (1427387312, 1427387312))
    os.utime(os.path.join('test', 'case17', 'A.h'), (1427987227, 1427987227))
    os.utime(os.path.join('test', 'case17', 'All.h'), (1427985886, 1427985886))
    os.utime(os.path.join('test', 'case17', 'ANew.h'), (1427987268, 1427987268))
    os.utime(os.path.join('test', 'case17', 'B.cpp'), (1427215320, 1427215320))
    os.utime(os.path.join('test', 'case17', 'B.h'), (1427218446, 1427218446))
    os.utime(os.path.join('test', 'case17', 'C.cpp'), (1427461578, 1427461578))
    os.utime(os.path.join('test', 'case17', 'C.h'), (1427987732, 1427987732))
    os.utime(os.path.join('test', 'case17', 'CNew.h'), (1427987320, 1427987320))
    os.utime(os.path.join('test', 'case17', 'OnlyB.h'), (1427214610, 1427214610))

    # test\case2 files
    os.utime(os.path.join('test', 'case2', 'A.cpp'), (1427387312, 1427387312))
    os.utime(os.path.join('test', 'case2', 'A.h'), (1427218226, 1427218226))
    os.utime(os.path.join('test', 'case2', 'All.h'), (1427389099, 1427389099))
    os.utime(os.path.join('test', 'case2', 'B.cpp'), (1427215320, 1427215320))
    os.utime(os.path.join('test', 'case2', 'B.h'), (1427218446, 1427218446))
    os.utime(os.path.join('test', 'case2', 'C.cpp'), (1427387371, 1427387371))
    os.utime(os.path.join('test', 'case2', 'C.h'), (1427215817, 1427215817))
    os.utime(os.path.join('test', 'case2', 'IncAll.h'), (1427220376, 1427220376))
    os.utime(os.path.join('test', 'case2', 'OnlyA.h'), (1427397043, 1427397043))
    os.utime(os.path.join('test', 'case2', 'OnlyB.h'), (1427214610, 1427214610))
    os.utime(os.path.join('test', 'case2', 'OnlyC.h'), (1427212284, 1427212284))

    # test\case3 files
    os.utime(os.path.join('test', 'case3', 'A.cpp'), (1427387312, 1427387312))
    os.utime(os.path.join('test', 'case3', 'A.h'), (1427218226, 1427218226))
    os.utime(os.path.join('test', 'case3', 'All.h'), (1427389099, 1427389099))
    os.utime(os.path.join('test', 'case3', 'B.cpp'), (1427215320, 1427215320))
    os.utime(os.path.join('test', 'case3', 'B.h'), (1427218446, 1427218446))
    os.utime(os.path.join('test', 'case3', 'C.cpp'), (1427461578, 1427461578))
    os.utime(os.path.join('test', 'case3', 'C.h'), (1427461573, 1427461573))
    os.utime(os.path.join('test', 'case3', 'IncAll.h'), (1427220376, 1427220376))
    os.utime(os.path.join('test', 'case3', 'OnlyA.h'), (1427397043, 1427397043))
    os.utime(os.path.join('test', 'case3', 'OnlyB.h'), (1427214610, 1427214610))
    os.utime(os.path.join('test', 'case3', 'OnlyC.h'), (1427212284, 1427212284))

    # test\case4 files
    os.utime(os.path.join('test', 'case4', 'A.cpp'), (1427387312, 1427387312))
    os.utime(os.path.join('test', 'case4', 'A.h'), (1427218226, 1427218226))
    os.utime(os.path.join('test', 'case4', 'All.h'), (1427463481, 1427463481))
    os.utime(os.path.join('test', 'case4', 'B.cpp'), (1427215320, 1427215320))
    os.utime(os.path.join('test', 'case4', 'B.h'), (1427218446, 1427218446))
    os.utime(os.path.join('test', 'case4', 'C.cpp'), (1427461578, 1427461578))
    os.utime(os.path.join('test', 'case4', 'C.h'), (1427461573, 1427461573))
    os.utime(os.path.join('test', 'case4', 'IncAll.h'), (1427220376, 1427220376))
    os.utime(os.path.join('test', 'case4', 'OnlyA.h'), (1427397043, 1427397043))
    os.utime(os.path.join('test', 'case4', 'OnlyB.h'), (1427214610, 1427214610))
    os.utime(os.path.join('test', 'case4', 'OnlyC.h'), (1427212284, 1427212284))

    # test\case5 files
    os.utime(os.path.join('test', 'case5', 'A.cpp'), (1427387312, 1427387312))
    os.utime(os.path.join('test', 'case5', 'A.h'), (1427218226, 1427218226))
    os.utime(os.path.join('test', 'case5', 'All.h'), (1427466505, 1427466505))
    os.utime(os.path.join('test', 'case5', 'B.cpp'), (1427215320, 1427215320))
    os.utime(os.path.join('test', 'case5', 'B.h'), (1427218446, 1427218446))
    os.utime(os.path.join('test', 'case5', 'C.cpp'), (1427461578, 1427461578))
    os.utime(os.path.join('test', 'case5', 'C.h'), (1427461573, 1427461573))
    os.utime(os.path.join('test', 'case5', 'IncAll.h'), (1427220376, 1427220376))
    os.utime(os.path.join('test', 'case5', 'OnlyA.h'), (1427397043, 1427397043))
    os.utime(os.path.join('test', 'case5', 'OnlyB.h'), (1427214610, 1427214610))
    os.utime(os.path.join('test', 'case5', 'OnlyC.h'), (1427212284, 1427212284))

    # test\case6 files
    os.utime(os.path.join('test', 'case6', 'A.cpp'), (1427387312, 1427387312))
    os.utime(os.path.join('test', 'case6', 'A.h'), (1427729547, 1427729547))
    os.utime(os.path.join('test', 'case6', 'All.h'), (1427729577, 1427729577))
    os.utime(os.path.join('test', 'case6', 'B.cpp'), (1427215320, 1427215320))
    os.utime(os.path.join('test', 'case6', 'B.h'), (1427218446, 1427218446))
    os.utime(os.path.join('test', 'case6', 'C.cpp'), (1427461578, 1427461578))
    os.utime(os.path.join('test', 'case6', 'C.h'), (1427461573, 1427461573))
    os.utime(os.path.join('test', 'case6', 'IncAll.h'), (1427220376, 1427220376))
    os.utime(os.path.join('test', 'case6', 'OnlyA.h'), (1427397043, 1427397043))
    os.utime(os.path.join('test', 'case6', 'OnlyB.h'), (1427214610, 1427214610))
    os.utime(os.path.join('test', 'case6', 'OnlyC.h'), (1427212284, 1427212284))

    # test\case7 files
    os.utime(os.path.join('test', 'case7', 'A.cpp'), (1427387312, 1427387312))
    os.utime(os.path.join('test', 'case7', 'A.h'), (1427729547, 1427729547))
    os.utime(os.path.join('test', 'case7', 'All.h'), (1427732250, 1427732250))
    os.utime(os.path.join('test', 'case7', 'B.cpp'), (1427215320, 1427215320))
    os.utime(os.path.join('test', 'case7', 'B.h'), (1427218446, 1427218446))
    os.utime(os.path.join('test', 'case7', 'C.cpp'), (1427461578, 1427461578))
    os.utime(os.path.join('test', 'case7', 'C.h'), (1427732235, 1427732235))
    os.utime(os.path.join('test', 'case7', 'IncAll.h'), (1427220376, 1427220376))
    os.utime(os.path.join('test', 'case7', 'OnlyA.h'), (1427397043, 1427397043))
    os.utime(os.path.join('test', 'case7', 'OnlyB.h'), (1427214610, 1427214610))
    os.utime(os.path.join('test', 'case7', 'OnlyC.h'), (1427212284, 1427212284))

    # test\case8 files
    os.utime(os.path.join('test', 'case8', 'A.cpp'), (1427387312, 1427387312))
    os.utime(os.path.join('test', 'case8', 'A.h'), (1427812117, 1427812117))
    os.utime(os.path.join('test', 'case8', 'All.h'), (1427812159, 1427812159))
    os.utime(os.path.join('test', 'case8', 'B.cpp'), (1427215320, 1427215320))
    os.utime(os.path.join('test', 'case8', 'B.h'), (1427218446, 1427218446))
    os.utime(os.path.join('test', 'case8', 'C.cpp'), (1427461578, 1427461578))
    os.utime(os.path.join('test', 'case8', 'C.h'), (1427812140, 1427812140))
    os.utime(os.path.join('test', 'case8', 'IncAll.h'), (1427220376, 1427220376))
    os.utime(os.path.join('test', 'case8', 'OnlyA.h'), (1427397043, 1427397043))
    os.utime(os.path.join('test', 'case8', 'OnlyB.h'), (1427214610, 1427214610))
    os.utime(os.path.join('test', 'case8', 'OnlyC.h'), (1427212284, 1427212284))

    # test\case9 files
    os.utime(os.path.join('test', 'case9', 'A.cpp'), (1427387312, 1427387312))
    os.utime(os.path.join('test', 'case9', 'A.h'), (1427813252, 1427813252))
    os.utime(os.path.join('test', 'case9', 'All.h'), (1427812159, 1427812159))
    os.utime(os.path.join('test', 'case9', 'B.cpp'), (1427215320, 1427215320))
    os.utime(os.path.join('test', 'case9', 'B.h'), (1427218446, 1427218446))
    os.utime(os.path.join('test', 'case9', 'C.cpp'), (1427461578, 1427461578))
    os.utime(os.path.join('test', 'case9', 'C.h'), (1427812140, 1427812140))
    os.utime(os.path.join('test', 'case9', 'IncAll.h'), (1427220376, 1427220376))
    os.utime(os.path.join('test', 'case9', 'OnlyB.h'), (1427214610, 1427214610))
    os.utime(os.path.join('test', 'case9', 'OnlyC.h'), (1427212284, 1427212284))