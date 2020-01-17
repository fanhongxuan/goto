all:
	g++ -o goto main.cpp log.c
	cp goto /home/ubuntu/fanhongxuan/bin/
	cp goto.sh /home/ubuntu/fanhongxuan/bin/
clean:
	rm goto
	rm /home/ubuntu/fanhongxuan/bin/goto
	rm /home/ubuntu/fanhongxuan/bin/goto.sh
