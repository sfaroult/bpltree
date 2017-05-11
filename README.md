# bpltree
With the companion btree program, bpltree is intended at making students better understand indices in databases.

The problem with explaining indices is that usually you have to explain B-trees in a theoretical and often abstract way, then you run a demo that is rarely convincing on a real database - rarely convincing because modern DBMS product scan tables very fast, especially when they are in memory, and because tables used during courses don't have the size of your average corporate table. Additionally, short of dumping index blocks, something you don't really want to do before beginners, there is no way to let students "see" how indices relate to data other than the CREATE INDEX statement.

Among the few things I have learned in a quarter of century of database consulting is the fact that young university graduates either ignore everything of indices, or believe that an index fixes magically all performance issues, and in the best of cases have no clue about composite indexes nor when an existing index cannot be used. These programs were written to help set the record straight.

I suggest you start with the companion program btree to explain how a B-Tree grows. The btree program only stores key values. With a small class, you can ask students to give their names and enter them one by one into the tree; you can ask them to preditc what will happen first. Or you can ask them for the name of cities to enter. You can also show what happens when you delete a key. If you want, you can preload some data when the program starts, so that you can start splitting nodes from the very beginning; it can read values from a text file containing one key per line. The index can be required to be unique (only accept distinct values) but isn't by default - in case several students would have the same name.

Once the B-Tree concepts are more or less understood, you can switch to the bpltree program that implements a B+Tree, in other words where leaf nodes contain values associated to keys. This is what a real database index looks like. With bpltree you are more or less supposed to load the values from a text file at start-up (which is completely optional with btree); although it is possible to enter (key, value) pairs by hand, possibly to illustrate the difference between a B-Tree and a B+Tree, reading a file is the normal way of operation. Each line is supposed to be composed of tab-separated fields (the separator has to be a single character but can be something else). A zipped sample file of actors and directors is provided, extracted from the sample database available at edu.konagora.com. I have restricted the people extracted (more than 600) to people who appear in 10 movies or more in my database, which biases the sample in favor of more famous but also more mature (or dead) people. You can of course use any text file you want or modify the one that is supplied.

My sample contains five fields:
<ol>
 <li>first-name (can be empty)</li>@
 <li>surname</li>
 <li>'Actor', 'Director', or 'Director and Actor'. Note that 'Actor' is gender-neutral here! (the gender isn't stored in my database)</li>
 <li>birth year</li>
 <li>death year (can be empty)</li>
</ol>

By default, the program uses as key the first field, and as associated value the offset of the line in the file. It's easy to explain that in a database index the address is a bit more complicated (in the case of Oracle it's a combination of file#, block# within the file and offset within the block) but that it's basically the same idea. Using the default option with the sample data file fails, because this program requires key values to be unique (I've been too lazy to implement a satisfactory deletion of keys when they aren't unique, and it wasn't very important for demonstration purposes). I suggest you start the program with option '-f 2,1' that says 'create a composite index with field#2 (surname) followed by field#1 (first name)'.

Once in the program, only a few commands are important:
<ul>
 <li><strong>show</strong> (or <strong>display</strong>, a synonym), which displays the index in a legible way</li>
 <li><strong>get</strong> that fetches lines from the file by using the index</li>
 <li><strong>scan</strong> that fetches lines from the file by scanning the file</li>
</ul>
<strong>get</strong> and <strong>scan</strong> display the rows fetched, as well as the time. If you process a lot of data and only want to focus on time difference, <strong>gettime</strong> and <strong>scantime</strong> operate in the same way but don't output the lines that were retrieved, only displaying the number the rows retrieved and the time taken (information also displayed by <strong>get</strong> and <strong>scan</strong>).

<strong>get</strong> and <strong>scan</strong> can fetch a single line, or a range of lines (note that <strong>scan</strong> always scans the file entirely, as it has no idea of what is unique).

A few examples will show what you can do - I assume the index created as suggested above:
<ul>
  <li><strong>get Wayne:John</strong>  Retrieve information about John Wayne</li>
  <li><strong>scan John:Wayne</strong>  Ditto, but by scanning the file; with <strong>scan</strong> values have to be provided either in the same order as in the file, or by specifing their position with <strong>@</strong> followed by a field #, eg <strong>scan Wayne@2:John@1</strong></li>
  <li><strong>get Ford</strong> Retrieve all information about people with the surname "Ford"; you can show that <strong>get John</strong> doesn't work, but that <strong>scan John</strong> does</li>
  <li><strong>get ,Aykroyd</strong> Retrieve all people with a name smaller than"Aykroyd"; <strong>scan ,Aykroyd@2</strong> does the same by scanning</li>
   <li><strong>get Le,Lu</strong> (resp <strong>scan Le@2,Lu</strong>) Retrieve all people with a name greater than "Le" and smaller (or equal too) "Luzzzzzzzz"; note that the filed position must be specified only once per key with <strong>scan</strong><li>
</ul>

You can point out that with range scans <strong>get</strong> always returns keys in alphabetical order, while <strong>scan</strong> returns them in the order in which they are stored in the file.

Note that <strong>scan</strong> allows searching on empty fields:

<strong>scan @1:@5</strong> will retrieve information about people who have neither a firstname nor a death year. 
  
It's important, I think, to show that an index is only usable if you query on the first field in the key (restarting the program by inverting the order of the keys might be a good idea), and displaying the index should drill into minds that if you apply a function to the surname (eg surname ending with whatever), then it's impossible to descend the index and therefore to use it.
