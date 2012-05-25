all: vmmengine
vmmengine: config.h funcTools.h funcTools.c vmmengine.c vix.h vixHandler.h vixVarStruct.h vm_basic_types.h vixHandler.c vixSession.h vixSession.c
	gcc funcTools.c md5.c vmmengine.c vixHandler.c vixSession.c -o vmmengine -lvixAllProducts -ldl
test: tmain.c
	gcc tmain.c funcTools.c vixHandler.c -o tmain -lvixAllProducts -ldl
clean:
	rm -f vmmengine

