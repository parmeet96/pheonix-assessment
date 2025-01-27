#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <json/json.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

#define SET_MODE _IOW('a','a',int)
#define SET_BLOCK_SYSCALL _IOW('b','b',struct ProcessInfo)
#define SET_BLOCK_PID _IOW('c','c',pid_t)
#define SET_LOG_SYSCALL_FSM _IOW('d','d',int)
#define GET_LOG_SYSCALL_FSM _IOR('e','e',int)

#define MODE_OFF 0
#define MODE_LOG 1
#define MODE_BLOCK 2
#define MODE_LOG_FSM 3
#define DEVICE_FILE "/dev/Char_device_driver"

struct ProcessInfo
{
	int pid;
	int syscall;
};

void print_helper()
{
	std::cout << "Helper \n ";
	std::cout << " --syscall <system_call_name>  Set the System Call name :: Open/Read/Write Followed by Kernel Modes \n";
	std::cout << " --block <pid>                      Set the Kernel Module to Block Mode \n";
	std::cout << " --log                         Set the Kernel Module to Log Mode \n";
	std::cout << " --off                         Set the Kernel Module to Off Mode \n";
	std::cout << " --file <fsm_file.json>        Use the FSM from Json File (to be used only with Log)\n";

}



std::vector<std::string> parse_fsm_json(std::string &filepath)
{
	std::ifstream file(filepath);
	if(!file.is_open())
	{
		throw std::runtime_error("Unable to open fsm file :: "+filepath);
	}

	Json::Value root;
	file >> root;

	if(!root.isArray())
	{
		throw std::runtime_error("Invalid JSON Format expecting array \n");
	}

	std::vector<std::string> fsm_states;

	for(const auto &state : root)
	{
		if(!state.isString())
		{
			throw std::runtime_error("Invalid JSON Format expecting String \n");

		}
		fsm_states.push_back(state.asString());
	}

	return fsm_states;

}


void send_ioctl_command(int fd, unsigned int cmd,int arg)
{
	if(ioctl(fd,cmd,arg)<0)
	{
		throw std::runtime_error("Invalid JSON Format expecting String \n");
	}

}

int main(int argc,char *argv[])
{
	if(argc <2)
	{
		print_helper();
		return 1;
	}

	int mode =-1;
	std::string syscall_name;
	std::string fsm_file;
	int pid =-1;
	bool use_fsm = false;

	for(int i=1;i<argc;i++)
	{
		std::string arg =argv[i];
		
		if(arg == "--syscall" && i+1 < argc)
		{
			syscall_name =argv[++i];
		}
		else if(arg == "--block")
		{
			mode = MODE_BLOCK;
			if(i+1 < argc )
			{
				std::string str_pid = argv[++i];
			 	pid =std::stoi(str_pid);	
			}
			else
			{
				 print_helper();
				 std::cout << "pid for the process should be provided \n";
			}
		}
		else if(arg == "--log")
		{
			mode = MODE_LOG;
		}
		else if(arg == "--off")
		{
			mode =MODE_OFF;
		}
		else if (arg == "--file" && i+1 < argc)
		{
			fsm_file = argv[++i];
			use_fsm =true;
			if(i+1 < argc && argv[++i] == "--log")
			{
				mode = MODE_LOG;
			}
			else
			{
				 print_helper();
				 std::cout << "--log should be used alway when using FSM Mode\n";
		                 return 1;
			}
		}
		else
		{
			 print_helper();
	                 return 1;
		}
	}

	int fd = open(DEVICE_FILE,O_RDWR);
	if(fd < 0)
	{
		perror("open");
		return 1;
	}

	try{
		if(mode !=-1 && mode != MODE_BLOCK && !use_fsm)
		{
			send_ioctl_command(fd,SET_MODE,mode);
			std::cout << "Mode set to " << (mode == MODE_OFF ? "OFF" : "LOG") << std::endl; 
		}

		if(mode !=-1  && mode == MODE_BLOCK )
		{

			ProcessInfo process_info;

			if(!syscall_name.empty() && pid !=-1)
			{
				process_info.pid = pid;
				if(syscall_name == "OPEN")
				{
					process_info.syscall=0;
					ioctl(fd,SET_BLOCK_SYSCALL,&process_info);
					std::cout << "Syscall set to  " << syscall_name << std::endl;
				}
				else if(syscall_name == "READ")
				{
					process_info.syscall=1;
					ioctl(fd,SET_BLOCK_SYSCALL,&process_info);
                                	std::cout << "Syscall set to  " << syscall_name << std::endl;
				}
				else if(syscall_name == "WRITE")
                        	{
					process_info.syscall=2;
                         	       ioctl(fd,SET_BLOCK_SYSCALL,&process_info);
                         	       std::cout << "Syscall set to  " << syscall_name << std::endl;
                        	}
			}
			else
			{
				std::cerr << "Error :: Either pid or syscall is empty cannot block process\n";
			}
		}

		if(use_fsm && mode == MODE_LOG)
		{
			std::vector<std::string> fsm_states = parse_fsm_json(fsm_file);

			if(fsm_states.empty())
			{
				throw std::runtime_error("Error :: FSM State is empty");
			}

			std::cout << "FSM Loaded  \n";

			int current_state=0;
			int state_pointer=-1;
			int read_pointer=-1;
			int i =0;
			//TODO :: Counter to take 10 commands from FSM integerate with Interrupt signal 
			//to come out of loop
			while(i <10)
			{
				if(fsm_states[current_state] == "OPEN")
                                {
					state_pointer=0;
                                        send_ioctl_command(fd,SET_LOG_SYSCALL_FSM,state_pointer);
                                        std::cout << "Syscall set to  " << fsm_states[current_state] << "through FSM" << std::endl;
                                }
                                else if(fsm_states[current_state] == "READ")
                                {
					state_pointer=1;
                                        send_ioctl_command(fd,SET_LOG_SYSCALL_FSM,state_pointer);
                                        std::cout << "Syscall set to  " << fsm_states[current_state] << "through FSM" << std::endl;
                                }
                                else if(fsm_states[current_state] == "WRITE")
                                {
					state_pointer=2;
                                       send_ioctl_command(fd,SET_LOG_SYSCALL_FSM,state_pointer);
                                       std::cout << "Syscall set to  " << fsm_states[current_state] << "through FSM" << std::endl;
                                }

				//TODO:: Replace with another logic
				sleep(3);
				read_pointer=-1;
				if(ioctl(fd,GET_LOG_SYSCALL_FSM,&read_pointer)<0)
				{
					perror("Failed to get value from kernel");
					close(fd);
					break;
				}
				if(read_pointer == state_pointer)
				{
					current_state=(current_state+1)%fsm_states.size();
					i++;
					std::cout << "Successfully Observed the syscall and read back from kernel moving to next state\n";
				}
				else
				{
					i++;
					std::cout << "Retrying with same FSM state for another try\n";
				}
			
			}

		
		}


	}
	catch(const std::exception &e)
	{
		std::cerr << "Error ::" << e.what() << std::endl;
	
	}
	close(fd);
	return 0;

}

