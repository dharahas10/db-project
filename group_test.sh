./db "select total, count(*) from class group by total"
./db "select total, count(student_name) from class group by total"
./db "select total, sum(total) from class group by total"
./db "select total, avg(total) from class group by total"

./db "select total, count(student_name) from class where total>600 group by total "
./db "select total, count(student_name) from class  where student_name='David' group by total "

./db "select gender, count(*) from class group by gender"
./db "select gender, sum(quiz_total) from class group by gender"
./db "select gender, avg(quiz_total) from class group by gender"
./db "select gender, avg(exams) from class group by gender"

./db "select gender, count(*) from class group by gender order by gender desc"
./db "select gender, count(*) from class group by gender order by gender"


./db "select * from class"