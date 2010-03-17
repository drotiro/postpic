#!/bin/bash
cd $(dirname $0)
java -Xmx512M -jar ../dist/PostPicSQL.jar jdbc:postgresql://localhost:5432/postpic postpic postpic
