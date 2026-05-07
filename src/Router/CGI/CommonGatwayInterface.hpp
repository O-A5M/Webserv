#ifndef COMMONGATEWAYINTERFACE_HPP
#define COMMONGATEWAYINTERFACE_HPP

#include "../../../inc/include.hpp"

class	CommonGatewayInterface {
	private:
		char	**_env_var;
		int		prepare_env(Request &request);
	public:
		CommonGatewayInterface(Request &request);
		~CommonGatewayInterface(void);
};
#endif