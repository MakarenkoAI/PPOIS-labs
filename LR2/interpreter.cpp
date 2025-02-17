
#include "interpreter.h"
using namespace std;

void machine::ifeq_label(string label)
{
	if (debug)
	{
		cout << "IFEQ";
		if ((*function_call_stack.begin()).second.ifeq_flag(debug))
		{
			cout << "True ifeq. Go to label\n";
			goto_label(label);
		}
		else
			cout << "Condition not met\n";
	}
	else
	{
		if ((*function_call_stack.begin()).second.ifeq_flag())
			goto_label(label);
	}
}

void machine::ifgr_label(string label)
{
	if (debug)
	{
		cout << "IFGR ";

		if ((*function_call_stack.begin()).second.ifgr_flag(debug))
		{
			cout << "Condition met. Go to label\n";
			goto_label(label);
		}
		else 
			cout << "Condition not met\n";
	}
	else
	{
		if ((*function_call_stack.begin()).second.ifgr_flag())
			goto_label(label);
	}

}

void machine::goto_label(string label)
{
	if (debug)
	{
		cout << "GOTO\t" << label << endl;

		string line;

		file.seekg((*function_call_stack.begin()).second.get_begin(), ios_base::beg);

		pair<cmatch, commands> command;

		do
		{
			getline(file, line);
			command = define_command(line);
			if (command.second == LABEL)
				if (command.first[1].str() == label)
					return;
			
		} while (command.second != FUNCTION_BRACKET_CLOSED);

		throw exceptions::name_err("No label with name " + label);
	}
	else
	{
		string line;

		file.seekg((*function_call_stack.begin()).second.get_begin(), ios_base::beg);

		pair<cmatch, commands> command;

		do
		{
			getline(file, line);
			command = define_command(line);

			if (command.second == LABEL)
				if (command.first[1].str() == label)
					return;
			
		} while (command.second != FUNCTION_BRACKET_CLOSED);

		throw exceptions::name_err("No label with name " + label);
	}
}

void machine::call_func(string name_func)
{
	if (debug)
	{ 
		cout << "CALL function\t" << name_func << endl;

		if (main_is_called)
		{
			if (name_func == "main") throw exceptions::syntax_err("Function	MAIN isn't callable.");

			int argument = (*function_call_stack.begin()).second.get_stack_head();

			function func_namespace(defined_functions[name_func].first, defined_functions[name_func].second);

			func_namespace.push_stack_head(argument);

			function_call_stack.push_front({ name_func, func_namespace });

			reader_checkpoints.push_front(file.tellg());
		}
		else
		{
			function main(defined_functions[name_func].first, defined_functions[name_func].second);

			function_call_stack.push_front({ name_func, main });

			main_is_called = true;
		}
		file.seekg((*function_call_stack.begin()).second.get_begin(), ios_base::beg);

		show_function_call_stack();
	}
	else
	{
		if (main_is_called)
		{
			if (name_func == "main")
				throw exceptions::syntax_err("Function MAIN isn't callable.");

			int argument = (*function_call_stack.begin()).second.get_stack_head();

			function func(defined_functions[name_func].first, defined_functions[name_func].second);

			func.push_stack_head(argument);

			function_call_stack.push_front({ name_func, func });

			reader_checkpoints.push_front(file.tellg());
		}
		else
		{
			function main(defined_functions[name_func].first, defined_functions[name_func].second);

			function_call_stack.push_front({ name_func, main });

			main_is_called = true;
		}
		file.seekg((*function_call_stack.begin()).second.get_begin(), ios_base::beg);
	}
}

void machine::return_func()
{
	if (debug)
	{ 
		cout << "RETURN for function \"" << (*function_call_stack.begin()).first << "\"" << endl;

		int value_to_return = (*function_call_stack.begin()).second.get_stack_head();

		function_call_stack.pop_front();

		if (function_call_stack.empty())
		{
			result = value_to_return;
			return;
		}

		file.seekg(*reader_checkpoints.begin(), ios_base::beg);

		reader_checkpoints.pop_front();

		(*function_call_stack.begin()).second.push_stack_head(value_to_return);

		cout << "Returned value:\t" << value_to_return << " to function \""<< (*function_call_stack.begin()).first << "\"" << endl;

		show_function_call_stack();
	}
	else
	{
		int return_value = (*function_call_stack.begin()).second.get_stack_head();

		if ((*function_call_stack.begin()).first == "main")
		{
			cout << "Stack of MAIN function:\n";
			(*function_call_stack.begin()).second.show_stack();
			result = return_value;
			function_call_stack.pop_front();
			return;
		}

		function_call_stack.pop_front();

		file.seekg(*reader_checkpoints.begin());
		
		reader_checkpoints.pop_front();
		
		(*function_call_stack.begin()).second.push_stack_head(return_value);
	}
}

void machine::callext_func(string label)
{
}

void machine::compile_functions()
{
	fstream book;

	book.open(filename);

	string line;

	while (getline(book, line))
	{
		cmatch function_matches;

		if (regex_match(line.c_str(), function_matches, command_templates[FUNCTION].first))
		{
			int function_begin, function_end;

			string function_name = function_matches[1].str();

			if (!function_matches[2].matched)
			{
				cmatch check_for_open;

				while (getline(book, line))
				{
					if (regex_match(line.c_str(), check_for_open, command_templates[FUNCTION_BRACKET_OPEN].first))	
						break;
					if (regex_match(line.c_str(), check_for_open, command_templates[COMMENT].first))	
						continue;
					throw exceptions::syntax_err("Error in \"" + function_name + "\" function.");
				}
			}

			function_begin = book.tellg();

			cmatch check_for_closed;

			while (getline(book, line))
			{
				if (regex_match(line.c_str(), check_for_closed, command_templates[FUNCTION_BRACKET_CLOSED].first)) 
					break;

				if (regex_match(line.c_str(), check_for_closed, command_templates[FUNCTION].first))
					throw exceptions::syntax_err(" Error in definition of \"" + function_name + "\" function."); 

				if (regex_match(line.c_str(), check_for_closed, command_templates[FUNCTION_BRACKET_OPEN].first))
					throw exceptions::syntax_err("Error in definition of \"" + function_name + "\" function."); 
			}

			if (check_for_closed.size() == 0) 
				throw exceptions::syntax_err("No \"}\" used in \"" + function_name + "\" function.");

			function_end = book.tellg();

			is_unique(function_name);

			defined_functions.emplace(function_name, pair<int, int>(function_begin, function_end));
		}
	}

	check_for_main();
}

void machine::is_unique(string function_name)
{
	for (auto function : defined_functions)
		if (function.first == function_name)
			throw exceptions::syntax_err("Fun�tion \"" + function_name + "\" already exists.");
	return;
}

void machine::check_for_main()
{
	for (auto function : defined_functions)
		if (function.first == "main")
			return;
	throw exceptions::interpret_err("Error. There is no MAIN");
}

pair<cmatch, machine::commands> machine::define_command(string& line)
{
	cmatch matches;

	for (auto command_template : command_templates)
		if (regex_match(line.c_str(), matches, command_template.first))
			return { matches, command_template.second };
		
	throw exceptions::syntax_err("No such command: \"" + line + "\".");
}

void machine::call_command(pair<cmatch, machine::commands> command_to_call)
{
	switch (command_to_call.second)
	{
	case machine::PUSH_DIGIT:
		(*function_call_stack.begin()).second.push(stoi(command_to_call.first[1].str()), debug);
		if (debug)
		{
			show_function_call_stack();
			system("pause");
		}
		break;

	case machine::PUSH_LABEL:
		(*function_call_stack.begin()).second.push(command_to_call.first[1].str(), debug);
		if (debug) 
		{
			show_function_call_stack();
			system("pause");
		}
		break;

	case machine::POP:
		(*function_call_stack.begin()).second.pop(debug);
		if (debug)
		{
			show_function_call_stack();
			system("pause");
		}
		break;

	case machine::POP_LABEL:
		(*function_call_stack.begin()).second.pop(command_to_call.first[1].str(), debug);
		if (debug)
		{
			show_function_call_stack();
			system("pause");
		}
		break;

	case machine::PEEK:
		(*function_call_stack.begin()).second.peek(command_to_call.first[1].str(), debug);
		if (debug)
		{
			show_function_call_stack();
			system("pause");
		}
		break;

	case machine::DUP:
		(*function_call_stack.begin()).second.dup(debug);
		if (debug)
		{
			show_function_call_stack();
			system("pause");
		}
		break;

	case machine::ADD:
		(*function_call_stack.begin()).second.add(debug);
		if (debug)
		{
			show_function_call_stack();
			system("pause");
		}
		break;

	case machine::SUB:
		(*function_call_stack.begin()).second.sub(debug);
		if (debug)
		{
			show_function_call_stack();
			system("pause");
		}
		break;

	case machine::DIV:
		(*function_call_stack.begin()).second.div(debug);
		if (debug)
		{
			show_function_call_stack();
			system("pause");
		}
		break;

	case machine::MUL:
		(*function_call_stack.begin()).second.mul(debug);
		if (debug)
		{
			show_function_call_stack();
			system("pause");
		}
		break;

	case machine::IFEQ:
		ifeq_label(command_to_call.first[1].str());
		if (debug)
		{
			show_function_call_stack();
			system("pause");
		}
		break;

	case machine::IFGR:
		ifgr_label(command_to_call.first[1].str());
		if (debug)
		{
			show_function_call_stack();
			system("pause");
		}
		break;

	case machine::GOTO_LABEL:
		goto_label(command_to_call.first[1].str());
		if (debug)
		{
			show_function_call_stack();
			system("pause");
		}
		break;

	case machine::CALL_FUNC:
		call_func(command_to_call.first[1].str());
		if (debug)
			system("pause");
		break;

	case machine::RETURN_FUNC:
		return_func();
		break;

	case machine::CALLEXT_FUNC:
		callext_func(command_to_call.first[1].str());
		break;

	case machine::LABEL:
		break;

	case machine::FUNCTION:
		throw exceptions::interpret_err("Error in rhe definition of commands");
		break;

	case machine::COMMENT:
		break;

	case machine::FUNCTION_BRACKET_OPEN:
		throw exceptions::syntax_err("Use { only to define function.");
		break;

	case machine::FUNCTION_BRACKET_CLOSED:
		throw exceptions::syntax_err("Function \"" + (*function_call_stack.begin()).first + "\" doesn't have RETURN");
		break;
	}
}

void machine::show_function_call_stack()
{
	cout << "Function call stack:\n";
	int i = 1;
	for (auto function : this->function_call_stack)
	{
		cout << endl;
		cout << i << ".\nFunction:\t" << function.first << "\n.....................................\n" << "\nStack:\n";
		function.second.show_stack();
		cout << "\n.....................................\n" << "Variables:\n";
		function.second.show_variables();
		i++;
	}
	cout << endl;
}

machine::machine(string filename, bool debug)
{
	this->filename = filename;

	file.open(filename);

	this->debug = debug;

	compile_functions();

	main_is_called = false;
}

void machine::interpret()
{
	string line;

	call_func("main");

	while (getline(file, line))
	{
		call_command(define_command(line));

		if (function_call_stack.empty()) 
			return;
	}

	throw exceptions::interpret_err("Unexpected end of file. Function call stack isn't empty");
}

int machine::get_result()
{
	return result;
}

machine::~machine()
{
	function_call_stack.clear();
	defined_functions.clear();
	filename.clear();
	file.close();
}
