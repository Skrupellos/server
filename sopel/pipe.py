"""
pipe.py - Sopel Module that reads from a UNIX named pipe and outputs lines to IRC

Reads lines according to the following format:
 recipient message
"""

import os
import sys
import time
import socket
import codecs
import threading
import traceback
import logging
import select
import fcntl
import errno
from copy import copy
from sopel.module import event, rule, priority
from sopel.config import ConfigurationError
from sopel.logger import get_logger

LOGGER = get_logger(__name__)

class PipeReader(threading.Thread):
	def warn(self, message):
		print(".... "+message)
		LOGGER.debug(message)
		sys.stdout.flush()
	
	
	def __init__(self):
		super(PipeReader, self).__init__()
		self.__running = True
		self.__pipe_r, self.__pipe_w = os.pipe()
	
	
	
	def run(self):
		self.__running = True
		self._do()
	
	
	
	def stop(self):
		self.__running = False
		
		print(self.__pipe_w)
		sys.stdout.flush()
		os.write(self.__pipe_w, b"\0")
		self.join()
	
	
	
	def _do(self):
		raise NotImplemented()
	
	
	
	def _waitLine(self, path):
		remainingData = b""

		for data in self.__waitData(path):
			if data is None:
				continue
			
			else:
				if remainingData is not None:
					lines = (remainingData + data).split(b"\n")
					## Only the first lines gets data (without an "\n")
					## prefixed, therefore only the first line can become to big
					if len(lines[0]) > 511:
						self.warn("Discarding oversized line '%s'" % lines[0])
						lines = lines[1:]
				
				## We have no valid data from the last time, indicating an error
				## has occurred. Therefore the first "line" still belongs to the
				## bogus data and has to be discarded.
				else:
					lines = data.split(b"\n")[1:]
				
				## Now lines contains only valid or no line (The last "line" is
				## not terminated and belongs to the next data)
				
				if len(lines) > 0:
					## Save the last line for the next round
					remainingData = lines[-1]
					
					## Return all other lines (all except for the last one)
					for line in lines[:-1]:
						yield line.decode('utf-8')
				
				else:
					remainingData = None
	
	
	
	def __waitData(self, path):
			## Open the reading end of the named pipe as non-blocking
			## The "+" causes the file to stay open if the other side disconnects
			fd = os.open(path, os.O_RDONLY|os.O_NONBLOCK)
			
			## Purge old data out of the named pipe
			while True:
				try:
					data = os.read(fd, 1024)
					
					if len(data) == 0:
						break
					
					self.warn(
						"Discarding old data from named pipe '%s'" %
						data
					)
				except OSError as ex:
					if ex.errno == errno.EAGAIN:
						break
					else:
						raise ex
			
			## Setup level triggered polling
			epoll = select.epoll()
			epoll.register(fd, select.EPOLLIN | select.EPOLLET)
			epoll.register(self.__pipe_r, select.EPOLLIN)
			
			## Poll to infinity
			firstEvent = True
			while self.__running:
				events = epoll.poll()
				
				for fileno, event in events:
					if fileno == fd:
						if len(events) == 0:
							yield None
						else:
							yield os.read(fd, 512)
					
					elif fileno == self.__pipe_r:
						os.read(fileno, 1)



class Pipe(PipeReader):
	def __init__(self, bot):
		super(Pipe, self).__init__()
		self.bot = bot
		self.pipe = None
	
	
	def parse_config(self, section):
		if section.file:
			self.pipe = section.file
		
		if not self.pipe:
			raise ConfigurationError('Missing pipe file for pipe {0}'.format(self.name))
	
	
	def _do(self):
		for line in self._waitLine(self.pipe):

			try:
				(recipient, message) = line.split(' ', 1)
				self.bot.msg(recipient, message, 5)
			except Exception as e:
				self.warn(u'error sending message: %s' % traceback.format_exc(e))



def setup(bot):
	## Get the defaults (or the only pipe) "[pipe]"
	#if 'pipe' in bot.config:
	bot.memory['pipe'] = Pipe(bot)
	bot.memory['pipe'].parse_config(bot.config.pipe)
	bot.memory['pipe'].start()




## Stop all pipes on quit
def shutdown(bot):
	bot.memory['pipe'].stop()
