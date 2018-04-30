all: ../src/fk-radio.pb.c ../src/fk-radio.pb.h

../src/fk-radio.pb.c ../src/fk-radio.pb.h: fk-radio.proto
	protoc --nanopb_out=../src fk-radio.proto

veryclean:

clean:
