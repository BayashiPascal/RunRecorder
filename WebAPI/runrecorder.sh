#!/bin/sh

action="action=$1"
if [ $# -eq 1 ]
then
  curl -d $action -H "Content-Type: application/x-www-form-urlencoded" -X POST https://localhost/RunRecorder/api.php
elif [ $# -eq 2 ]
then
  data=$2
  curl -d $action -d "${data}" -H "Content-Type: application/x-www-form-urlencoded" -X POST https://localhost/RunRecorder/api.php
fi
echo ""
