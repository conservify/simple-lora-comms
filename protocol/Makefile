all: src/fk-radio.pb.c src/fk-radio.pb.h

node_modules/.bin/pbjs:
	npm install

src/fk-radio.pb.c src/fk-radio.pb.h: fk-radio.proto
	protoc --nanopb_out=./src fk-radio.proto

veryclean:

clean:
