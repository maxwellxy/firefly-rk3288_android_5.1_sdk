/*
 * xml_helper.h
 *
 *  Created on: 2013-4-27
 *      Author: mmk
 */

#ifndef XML_HELPER_H_
#define XML_HELPER_H_

typedef void (*handlerFun) (char *command, int argc, char **argv);

int parse_area(FILE *f,
		char *area,
		char *local,
		char *language,
		char *operator,
		char *reserve,
		handlerFun handler);
int parse_device(FILE *f, char *dev, char *type, handlerFun handler);

#endif /* XML_HELPER_H_ */
