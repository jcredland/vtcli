/*
 Command-line editor for straightforward ValueTree compatible XML.  Designed as 
 a command-line editor for Introjucer jucer files, but undoubtedly has other 
 uses.
 
 Designed for use in scripts so it: 
 - has minimal output. 
 - returns an error code on failure.
*/

#include "../JuceLibraryCode/JuceHeader.h"
#include <iostream>

using namespace std; /* yep. really. */


void displayHelp()
{
    cout << "vtcli <xml_file_name> options ..." << endl;
    cout << "--node-name (or -n) <name> (repeat as required to navigate the tree)" << endl;
    cout << "--node-index (or -i) <number> (find a child by number rather than name)" << endl;
    cout << "-np <node_name> <property> <value_to_match> (select a node with a property that matches a particular value" << endl;
    cout << "--read <name_of_property>" << endl;
    cout << "--write <name_of_property> <value_to_write>" << endl;
    cout << "--list-nodes (show all the nodes)" << endl;
    cout << "--list-properties (show all the property names)" << endl;
    cout << "-l (list nodes and properties.  handy for debugging.)" << endl;
    cout << "-x show the XML from this point in the tree" << endl;
    cout << endl;
    cout << "Example: vtcli file.xml --node-name \"tag\" --node-name \"subtag\" --read \"size\"" << endl;
}

static bool shouldSaveFlag = false;

class Step
{
public:
    virtual ~Step() {}
    virtual ValueTree operator() (ValueTree &input) = 0;
private:
};

class ListNodesStep : public Step
{
public:
    ListNodesStep() {}
    ValueTree operator()(ValueTree & input) override
    {
        int numChildren = input.getNumChildren();

        for (int i = 0; i < numChildren; ++i)
            cout << input.getChild(i).getType().toString() << endl;

        return input;
    }
};

class ListPropertiesStep : public Step
{
public:
    ListPropertiesStep() {}
    ValueTree operator()(ValueTree & input) override
    {
        int numProps = input.getNumProperties();

        for (int i = 0; i < numProps; ++i)
            cout << input.getPropertyName(i).toString() << endl;

        return input;
    }
};

class NodeNameWithPropertyStep : public Step
{
public:
    NodeNameWithPropertyStep(const Identifier & node, const Identifier & property, var valueToMatch)
    :
    nodeName(node),
    propertyName(property),
    value(valueToMatch)
    {}

    ValueTree operator()(ValueTree & input) override
    {
        ValueTree subTree = input.getChildWithProperty(propertyName, value);

        if (subTree == ValueTree::invalid)
        {
            cout << "error: node not found, when looking for name '" << nodeName.toString() << "' with property '";
            cout << propertyName.toString() << "' equal to '" << value.toString() << "'" << endl;
            cout << "valid nodes were:" << endl;

            ListNodesStep listNodesStep;
            listNodesStep(input);
        }

        return subTree;
    }
private:
    Identifier nodeName;
    Identifier propertyName;
    var value;
};

class DisplayTree : public Step
{
public:
    DisplayTree() {}
    ValueTree operator()(ValueTree & input) override
    {
        cout << endl;
        cout << input.toXmlString();
        cout << endl;
        return input;
    }
};

class NodeNameStep : public Step
{
public:
    NodeNameStep(const Identifier & node) : nodeName(node) {}
    ValueTree operator()(ValueTree & input) override
    {
        ValueTree subTree = input.getChildWithName(nodeName);

        if (subTree == ValueTree::invalid)
        {
            cout << "error: not found: " << nodeName.toString() << endl;
            cout << "valid nodes were:" << endl;

            ListNodesStep listNodesStep;
            listNodesStep(input);
        }

        return subTree;
    }
private:
    Identifier nodeName;
};


class NodeIndexStep : public Step
{
public:
    NodeIndexStep(const Identifier & node) : nodeName(node) {}
    ValueTree operator()(ValueTree & input) override
    {
        ValueTree subTree = input.getChildWithName(nodeName);

        if (subTree == ValueTree::invalid)
            cout << "error: index out of range" << endl;

        return subTree;
    }
private:
    Identifier nodeName;
};


class WriteStep : public Step
{
public:
    WriteStep(const Identifier & property, var v) : propertyName(property), value(v) {}
    ValueTree operator()(ValueTree & input) override
    {
        input.setProperty(propertyName, value, nullptr);
        shouldSaveFlag = true;
        return input;
    }
private:
    Identifier propertyName;
    var value;
};


class ReadStep : public Step
{
public:
    ReadStep(const Identifier & property) : propertyName(property) {}
    ValueTree operator()(ValueTree & input) override
    {
        if (! input.hasProperty(propertyName))
        {
            cout << "error: property not found" << endl;
            return ValueTree::invalid;
        }
        cout << input[propertyName].toString() << endl;
        return input;
    }
private:
    Identifier propertyName;
    var value;
};


class Navigator
{
public:
    Navigator(const String & fileName)
    {
        inputFile = File (File::getCurrentWorkingDirectory().getChildFile(fileName));
    }

    int navigateThroughArgs(StringArray & args)
    {
        {
            FileInputStream vtInputStream (inputFile);

            if (! vtInputStream.openedOk())
            {
                cerr << "file open error :" << inputFile.getFullPathName() << endl;
                return 1;
            }

            XmlDocument xmlDoc (inputFile);
            ScopedPointer<XmlElement> xmlRootElement = xmlDoc.getDocumentElement();
            tree = ValueTree::fromXml(*xmlRootElement);
        }

        while (args.size() > 0)
        {
            if (processNextParameter(args) != 0)
                return 1;
        }

        ValueTree currentNode (tree);

        for (auto s: steps)
        {
            currentNode = (*s)(currentNode);

            if (currentNode == ValueTree::invalid)
                return 1;
        }

        if (shouldSaveFlag)
            return saveTree();
        else
            return 0;
    }

    bool isTreeLoaded;

private:

    int saveTree()
    {
        ScopedPointer<XmlElement> x = tree.createXml();
        String xmlDocument = x->createDocument(String::empty);

        if (! inputFile.replaceWithText(xmlDocument))
        {
            cerr << "vtcli error: writing stream " << inputFile.getFullPathName() << endl;
            return 1;
        }
        else
        {
            return 0;
        }
    }

    int processNextParameter(StringArray & args)
    {
        auto & a = args[0];

        if (a == "--list-nodes")
        {
            steps.add(new ListNodesStep());
            args.remove(0);
        }
        else if (a == "--list-properties")
        {
            steps.add(new ListPropertiesStep());
            args.remove(0);
        }
        else if (a == "-l")
        {
            steps.add(new ListNodesStep());
            steps.add(new ListPropertiesStep());
            args.remove(0);
        }
        else if (a == "-x")
        {
            steps.add(new DisplayTree());
            args.remove(0);
        }
        else if ((a == "--node-name" || a == "-n") && args.size() > 1)
        {
            steps.add(new NodeNameStep(args[1]));
            args.removeRange(0, 2);
        }
        else if ((a == "--node-index" || a == "-i") && args.size() > 1)
        {
            steps.add(new NodeIndexStep(args[1]));
            args.removeRange(0, 2);
        }
        else if (a == "--read" && args.size() > 1)
        {
            steps.add(new ReadStep(args[1]));
            args.removeRange(0, 2);
        }
        else if (a == "--write" && args.size() > 2)
        {
            steps.add(new WriteStep(args[1], args[2]));
            args.removeRange(0, 3);
        }
        else if (a == "-np" && args.size() > 3)
        {
            steps.add(new NodeNameWithPropertyStep(args[1], args[2], args[3]));
            args.removeRange(0, 4);
        }
        else
        {
            cout << "error in argument: " << args[0] << endl;
            displayHelp();
            return 1;
        }

        return 0;
    }
    File inputFile;
    OwnedArray<Step> steps;
    ValueTree tree;
};


int main (int argc, char* argv[])
{
    StringArray args(argv, argc);

    if (args.size() == 1)
    {
        displayHelp();
        return 1;
    }

    String programName = args[0];
    args.remove(0);

    Navigator navigator(args[0]);
    args.remove(0);

    int result = navigator.navigateThroughArgs(args);

    return result;
}
