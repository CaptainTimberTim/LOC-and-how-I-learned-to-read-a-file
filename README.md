# LOC or: How I learned to stop UI-Programming and Love the Prompt

Is a small tool that reads all code files in a given directory and it's subdirectories, counts the linebreaks -once with whitespaces and once without-, 
sorts them and lastly lists them in descending order.
Additionally, it tallies all file LOCs up and gives a total count at the bottom.<br>
The maximum amount of files is 1.5k at the moment. After that it stops collecting more files and lists the found ones. Filepaths longer than 25 symbols before the filename will also be shortened to keep length managable.
<br><br>
To add or remove file extensions that should be included -or not- the file "LOCFileExtensions.save" in th "data" folder can be edited. 
The specification is extremely simple. On each line _one_ file extension is written with no spaces or anything else.
<br><br>
The base principle for this project is to do _almost_ everything myself. The only libraries included are the stb_image.h and stb_truetype.h for image and font loading.
Everything else is written by me. With the rare code snippets here and there taken from somewhere else. <br>
Many things are hardcoded at this point and may fail for extremely large codebases. 
I tested it on my whole programming directory which consists of 1484 files amounting to around 500k lines of code. 

<br><br><br>
# Building the repository
To build this project, the only thing required is the Visual Studio compiler.<br>
I have a "build.bat" in the code directory, which I use for building.<br>
In the "build" directory is a Visual Studio project file, which is only used for debugging. <br><br>
To use the project exactly like me all files in the base directory need their paths changed (the shortcut path as well). 
That said, it is really not necessary to do.


