
# XML / ValueTree editor

This tool allows you to carry out operations on XML files written using 
the JUCE ValueTree class. 

I wrote it to enable me to edit .jucer files from build scripts, but it 
probably has some other uses in other applications. 

# Basics

You provide a suitable file in the ValueTree XML format.  Then using 
the navigation options (--node-name --node-index -np) you navigate
down the tree to the node you are after. 

Then you can read and write properties with the --read and --write
flags. 

It wouldn't be a massive leap of programming to have it also create
new nodes, but I haven't needed that so far and haven't added the code. 

# Other options
-l and -x are provided to help with the command line navigation of the
tree. 

# Options

vtcli <xml_file_name> options ...
--node-name (or -n) <name> (repeat as required to navigate the tree)
--node-index (or -i) <number> (find a child by number rather than name)
-np <node_name> <property> <value_to_match> (select a node with a property that matches a particular value
--read <name_of_property>
--write <name_of_property> <value_to_write>
--list-nodes (show all the nodes)
--list-properties (show all the property names)
-l (list nodes and properties.  handy for debugging.)
-x show the XML from this point in the tree

# Examples

Here it is being used to find and then change a value in the vtcli.jucer file: 

     $ ./vtcli vtcli.jucer --node-name MODULES -x

    <?xml version="1.0" encoding="UTF-8"?>

    <MODULES>
      <MODULES id="juce_core" showAllCode="1" useLocalCopy="0"/>
      <MODULES id="juce_data_structures" showAllCode="1" useLocalCopy="0"/>
      <MODULES id="juce_events" showAllCode="1" useLocalCopy="0"/>
    </MODULES>

     $ ./vtcli vtcli.jucer --node-name MODULES -np MODULES id juce_core --write useLocalCopy 1
     $ ./vtcli vtcli.jucer --node-name MODULES -x

    <?xml version="1.0" encoding="UTF-8"?>

    <MODULES>
      <MODULES id="juce_core" showAllCode="1" useLocalCopy="1"/>
      <MODULES id="juce_data_structures" showAllCode="1" useLocalCopy="0"/>
      <MODULES id="juce_events" showAllCode="1" useLocalCopy="0"/>
    </MODULES>

