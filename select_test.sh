./db "select * from class where student_name='Frank'"

./db "select * from class where student_name<'Good Student'"
echo "select * from class where gender is not null and total > 600"
./db "select * from class where gender is not null and total > 600"

echo "select * from class where gender is not null or total > 600"
./db "select * from class where gender is not null or total > 600"

echo "select * from class where gender is not null or total > 600 order by student_name desc"
./db "select * from class where gender is not null or total > 600 order by student_name desc"

echo "select * from class where gender is not null or total > 600 order by total desc"
./db "select * from class where gender is not null or total > 600 order by total desc"

echo "select AVG(total) from class"
./db "select AVG(total) from class"

echo "select SUM(total) from class"
./db "select SUM(total) from class"

echo "select COUNT(Student_Name) from class"
./db "select COUNT(Student_Name) from class"

echo "select COUNT(*) from class"
./db "select COUNT(*) from class"

echo "select student_name, gender, Exams from class"
./db "select student_name, gender, Exams from class"