if [ ! -d "bin" ]; then
    mkdir bin
else
	rm bin/*
fi

g++ -g -O0 -I . -o bin/interrupts_EP interrupts_101302106_101315124_EP.cpp 
#g++ -g -O0 -I . -o bin/interrupts_RR interrupts_101302106_101315124_RR.cpp
#g++ -g -O0 -I . -o bin/interrupts_EP_RR interrupts_101302106_101315124_EP_RR.cpp

./bin/interrupts_EP ./input_files/test1.txt 
#./bin/interrupts_RR ./input_files/test1.txt
#./bin/interrupts_EP_RR ./input_files/test1.txt
