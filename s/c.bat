echo off

if [%1] == [] GOTO EmptyCommitMessage

git add .

git commit -m %1

GOTO Finish

:EmptyCommitMessage

echo Empty Commit Message

:Finish

