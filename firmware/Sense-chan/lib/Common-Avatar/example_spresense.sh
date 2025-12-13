#!/bin/bash
rm -rf ./src/example
cp -r ./examples/spresense_avatar ./src/example
for f in ./src/example/*.ino; do
  mv "$f" "${f%.ino}.cpp"
done

