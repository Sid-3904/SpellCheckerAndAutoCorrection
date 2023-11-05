# SpellCheckerAndAutoCorrection
To run the program first compile it using "gcc -g -pthread .\spellChecker.c" and then type "./a.exe". Now it will offer user 5 options.

(1) The first option is for spell checking a few sentences given by user in terminal. It will identify the wrong words and print the top MAX_SUGGESTIONS for each incorrect word. Initially MAX_SUGGESTIONS is set to 10, user can change it as per his/her convenience in the spell.h header file.

(2) The second option is to compare accuracy vs speed of tries and bloom filter data structures when an input file "input.txt" is given for spell checking. In order to make bloom filter more efficient or less, user can change the number of hashes or size of bit array as per his/her convenience in spell.h header file.

(3) The third option is for reading the input file "input.txt" and find all the incorrect words and print their suggestions with the help of multi-threading. User can change the input file as per his/her convenience.

(4) The fourth option is for reading the input file "input.txt" and find all the incorrect words and print their suggestions with the help of LRU Cache. User can change the input file as per his/her convenience.

(5) To quit the program.

This is a robust spell checking program which inculcates various techniques to make the spell checking process as fast and as accurate as possible.

Thank you