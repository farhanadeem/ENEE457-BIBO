#!/bin/bash

dir_name=${PWD##*/}
ext=".log"
f_name="${dir_name}${ext}"

message1="- The output of your program is as follows"
message2="- The expected output is as follows"

echo ""
echo ""

# Clean any previoulsy built files and make a fresh one
make clean
make

# Remove any created gradebook files
rm mygradebook
setup -N mygradebook

# Ask the user the input the key generated by the program
echo ""
read -p "Input The Key Printed Above : " key
echo ""
echo "Key is: $key"
echo ""

gradebookadd -N mygradebook -K $key -AS -FN John -LN Smith
gradebookadd -N mygradebook -K $key -AS -FN Russell -LN Tyler
gradebookadd -N mygradebook -K $key -AS -FN Ted -LN Mason
gradebookadd -N mygradebook -K $key -AS -FN Peter -LN Parker

gradebookadd -N mygradebook -K $key -AA -AN Midterm -P 100 -W 0.25
gradebookadd -N mygradebook -K $key -AG -AN Midterm -FN John -LN Smith -G 95
gradebookadd -N mygradebook -K $key -AG -AN Midterm -FN Russell -LN Tyler -G 80
gradebookadd -N mygradebook -K $key -AG -AN Midterm -FN Ted -LN Mason -G 90
gradebookadd -N mygradebook -K $key -AG -AN Midterm -FN Peter -LN Parker -G 70

gradebookadd -N mygradebook -K $key -AA -AN Final -P 200 -W 0.5
gradebookadd -N mygradebook -K $key -AG -AN Final -FN John -LN Smith -G 180
gradebookadd -N mygradebook -K $key -AG -AN Final -FN Russell -LN Tyler -G 190
gradebookadd -N mygradebook -K $key -AG -AN Final -FN Ted -LN Mason -G 150
gradebookadd -N mygradebook -K $key -AG -AN Final -FN Peter -LN Parker -G 140

gradebookadd -N mygradebook -K $key -AA -AN Project -P 50 -W 0.25
gradebookadd -N mygradebook -K $key -AG -AN Project -FN John -LN Smith -G 48
gradebookadd -N mygradebook -K $key -AG -AN Project -FN Russell -LN Tyler -G 49
gradebookadd -N mygradebook -K $key -AG -AN Project -FN Ted -LN Mason -G 50
gradebookadd -N mygradebook -K $key -AG -AN Project -FN Peter -LN Parker -G 40

# Test 1 - Displaying grades for 'Midterm' assignment in alphabetical order
echo "** Test 1 **" >> $f_name
echo $message1 >> $f_name
gradebookdisplay -N mygradebook -K $key -PA -A -AN Midterm >> $f_name
echo $message2 >> $f_name
res=$'(Mason, Ted, 90)\n(Parker, Peter, 70)\n(Smith, John, 95)\n(Tyler, Russell, 80)\n'
echo "$res" >> $f_name
echo""

# Test 2 - Displaying grades for 'Final' assignment in order of grades
echo "** Test 2 **" >> $f_name
echo $message1 >> $f_name
gradebookdisplay -N mygradebook -K $key -PA -G -AN Final >> $f_name
echo $message2 >> $f_name
res=$'(Tyler, Russell, 190)\n(Smith, John, 180)\n(Mason, Ted, 150)\n(Parker, Peter, 140)\n'
echo "$res" >> $f_name
echo""

# Test 3 - Displaying all assignment grades for student 'John Smith'
echo "** Test 3 **" >> $f_name
echo $message1 >> $f_name
gradebookdisplay -N mygradebook -K $key -PS -FN John -LN Smith >> $f_name
echo $message2 >> $f_name
res=$'(Midterm, 95)\n(Final, 180)\n(Project, 48)\n'
echo "$res" >> $f_name
echo""

# Test 4 - Displaying final grades for all students in order of grades
echo "** Test 4 **" >> $f_name
echo $message1 >> $f_name
gradebookdisplay -N mygradebook -K $key -PF -G >> $f_name
echo $message2 >> $f_name
res=$'(Smith, John, 0.9275)\n(Tyler, Russell, 0.92)\n(Mason, Ted, 0.85)\n(Parker, Peter, 0.725)\n'
echo "$res" >> $f_name
echo""

gradebookadd -N mygradebook -K $key -DA -AN Project

# Test 5 - Displaying final grades for all students in alphabetical order after deleting 'Project' assignment
echo "** Test 5 **" >> $f_name
echo $message1 >> $f_name
gradebookdisplay -N mygradebook -K $key -PF -A >> $f_name
echo $message2 >> $f_name
res=$'(Mason, Ted, 0.6)\n(Parker, Peter, 0.525)\n(Smith, John, 0.6875)\n(Tyler, Russell, 0.675)\n'
echo "$res" >> $f_name
echo""

gradebookadd -N mygradebook -K $key -AA -P 50 -W 0.125 -AN Project >> $f_name
#read -p "Press Any Key to Enter Hidden Tests" temp

###############################################
############ Some Hidden Tests ################
echo ""
# Test 6 - Wrong key is provided : Wrong
echo "** Test 6 **" >> $f_name
echo $message1 >> $f_name
gradebookadd -N mygradebook -K 45ea448f -AA -P 50 -W 0.125 -AN Project >> $f_name
echo $message2 >> $f_name
echo 'invalid' >> $f_name
res=$'--> This is an invalid command, because key is wrong.\n'
echo "$res" >> $f_name
echo"" 

# Test 7 - Add the assinment 'Project' but incorrect ordering : Wrong
echo "** Test 7 **" >> $f_name
echo $message1 >> $f_name
gradebookadd -K $key -N mygradebook -AA -P 50 -W 0.125 -AN Project >> $f_name
echo $message2 >> $f_name
echo 'invalid' >> $f_name
res=$'--> This is an invalid setting, because order of arguments is incorrect.\n'
echo "$res" >> $f_name
echo"" 

# Test 8 - Add repeated ordering (Project Y should be created) : OK
echo "** Test 8 **" >> $f_name
echo $message1 >> $f_name
gradebookadd -N mygradebook -K $key -AA -AN Project_X -AN Projec_Y -P 50 -W 0.125 >> $f_name  
echo $message2 >> $f_name
res=$'--> This is a valid setting, because there can be multiple settings and the last one should be taken.\n'
echo "$res" >> $f_name
echo"" 

# Test 9 - Add the assignment but one parameter is not given : Wrong
echo "** Test 9 **" >> $f_name
echo $message1 >> $f_name
gradebookadd -N mygradebook -K $key -AA -P 50 -W 0.25 >> $f_name
echo $message2 >> $f_name
echo 'invalid' >> $f_name
res=$'--> This is an invalid setting, because the assigment name is not given.\n'
echo "$res" >> $f_name
echo"" 

# Test 10 - Conflicting Assignment and Add student : Wrong
echo "** Test 10 **" >> $f_name
echo $message1 >> $f_name
gradebookadd -N mygradebook -K $key -AA -AN BiggerProject -AS -FN Smithson -LN Kyla -P 50 -W 0.25 >> $f_name
echo $message2 >> $f_name
echo 'invalid' >> $f_name
res=$'--> This is an invalid setting, because commands are conflicting.\n'
echo "$res" >> $f_name
echo"" 

# Test 11 - Adding an existing name of student : Wrong
echo "** Test 11 **" >> $f_name
echo $message1 >> $f_name
gradebookadd -N mygradebook -K $key -AS -FN Peter -LN Parker >> $f_name
echo $message2 >> $f_name
echo 'invalid' >> $f_name
res=$'--> This is an invalid setting, because a student with the same name exists.\n'
echo "$res" >> $f_name
echo"" 

# Test 12 - Adding an assignment with an existing name : Wrong
echo "** Test 12 **" >> $f_name
echo $message1 >> $f_name
gradebookadd -N mygradebook -K $key -AA -P 50 -W 0.125 -AN Final >> $f_name
echo $message2 >> $f_name
echo 'invalid' >> $f_name
res=$'--> This is an invalid setting, because an assigment with the same name exists.\n'
echo "$res" >> $f_name
echo"" 

# Test 13 - Adding a student name with a number in between : Wrong
echo "** Test 13 **" >> $f_name
echo $message1 >> $f_name
gradebookadd -N mygradebook -K $key -AS -FN Jo2hn -LN Smith >> $f_name
echo $message2 >> $f_name
echo 'invalid' >> $f_name
res=$'--> This is an invalid setting, because name of students can only be alphabetic characters.\n'
echo "$res" >> $f_name
echo""

# Test 14 - Adding an assignment with non-alphabetical and non-numeric character : Wrong
echo "** Test 14 **" >> $f_name
echo $message1 >> $f_name
gradebookadd -N mygradebook -K $key -AA -AN Midterm_2 -P 100 -W 0.25 >> $f_name
echo $message2 >> $f_name
echo 'invalid' >> $f_name
res=$'--> This is an invalid setting, because name of assignments can only be alphabetic characters and numbers.\n'
echo "$res" >> $f_name
echo""

# Test 15 - Displaying a non-existing student's grade : Wrong
echo "** Test 15 **" >> $f_name
echo $message1 >> $f_name
gradebookdisplay -N mygradebook -K $key -PS -FN John -LN Tyler >> $f_name
echo $message2 >> $f_name
echo 'invalid' >> $f_name
res=$'--> This is an invalid setting, because this student does not exist.\n'
echo "$res" >> $f_name
echo""

###############################################

# clean the executables
make clean