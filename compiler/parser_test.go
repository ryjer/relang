import(
	f "fmt"
	"math"
)
+ var(
	x int = 1 +2
	y = 2
	z uint
)
var base int = 100
const x,y int = 8,9
+type user 结构体{
	next ->user
	+结果 枚举体{
		成功=0
		失败
	}
	name [16]char
	-u 共用体{
		age int8、height int
	}
}
{
	// 分支语句
	if a>b {
		me · age += 1
	} else {
		return i/2
	}	
	match x {
		|1: x++
		|'a':
			x += 2
		|2,3: x--
		|4,5,6,7: i++
	}
	continue L1
	break x
	// 循环语句
	while a > 1 {
		a--
	}
	for i:=1; i<4; i++ {
		a[i]++
	}
	循环 {
		i+=1
	} 直到 i = 3
	label test:  goto test
	defer add(x, y)
	return 1;
}
{
}
add(a+b,-i,x[j],this.age,5,6,7++,8,9 的尺寸,10,11,12)
// arr[x+y] += -i + j + add()
// -i
// a ∧= 若 -a+b*c > x%y 则 -~!me.age[4]->++-- + 1 else true
