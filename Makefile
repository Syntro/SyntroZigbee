#
# Recursive Makefile for all Syntro Zigbee components
#

COMPONENTS = SyntroZigbeeGateway \
	SyntroZigbeeDemo \
	ZigbeeTestNode
	
build:
	for dir in $(COMPONENTS); do \
		(cd $$dir; ${MAKE}); \
	done

config:
	for dir in $(COMPONENTS); do \
		(cd $$dir; qmake); \
	done

clean:
	for dir in $(COMPONENTS); do \
		(cd $$dir; ${MAKE} clean); \
	done 

distclean:
	for dir in $(COMPONENTS); do \
		(cd $$dir; rm -rf Makefile* debug release Debug Release GeneratedFiles; rm -f $$(ls Output/* | grep -v .ini)); \
	done

