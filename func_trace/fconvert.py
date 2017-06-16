#!/usr/bin/python

################################   useage   ##########################
#1. setup build env for android
#2. link the symbols to current,eg:ln -s /home/wanggongzhen/new_camera/out/target/product/lc1861evb_arm64/symbols symbols
#3. adb shell pull /data/local/log/trace ./trace
#4. ln -s trace/trace_xxx.txt trace.txt;
#	ln -s trace/maps_xxx.txt maps.txt; 
#5.	exec this file, ./fconvert.py
################################   end   ################################


import sys
import xml.dom.minidom
import commands

symbols_dir = './symbols'


class func_ex:
	'trace func enter and exit'
	def __init__(self , strfunc):
		self.addr = 0x0
		self.liboffset = 0x0
		self.enter = -1
		self.name = '-'
		self.info=''
		self.libname = '-'
		self.tid = 0
		self.sec=0
		self.nsec=0
		self.parse(strfunc)
		
	def parse(self , strfunc):	
		try:
			[enter , tid , addr , sec , nsec] = strfunc.split()
			if enter == 'e':
				self.enter = 1
			elif enter == 'x':
				self.enter = 0
			else:
				print 'parse func error:'+strfun
				self.enter = -1
			self.tid = tid
			self.addr = int(addr,16)
			self.sec = int(sec,10)
			self.nsec = int(nsec,10)
		except:
			pass
		
		
		
class func_thread:
	'all function in one thread'
	def __init__(self , tid):
		self.funcs = []
		self.tid = tid


class func_map:
	def __init__(self , strmap):
		self.start = 0x0
		self.end = 0x0
		self.libname=""
		self.parse(strmap)
	def parse(self , strmap):
		try:
			[address,prop,pgoffset,devid,inodeid,libname] = strmap.split()
			[start , end] = address.split('-')
			self.start = int(start,16)
			self.end = int(end, 16)
			self.libname = libname
		except:
			pass
	
class func_maps:
	'maps for libs'
	def __init__(self , filename):
		self.maps = []
		self.parsefile(filename)
		
	def parsefile(self , filename):
		infile = open(filename)
		for line in infile:
			map = func_map(line)
			self.maps.append(map)
		infile.close()
		
	def find(self , addr):
		for m in self.maps:
			if addr > m.start and addr < m.end:
				return m
		return None
	
	

		
class func_trace:
	'convert trace.txt to trace.xml'
	def __init__(self):
		self.threads = {}
		self.funccache = {}
		
	def read(self,filename):
		infile = open(filename)
		for line in infile:
			f = func_ex(line)
			if not self.threads.has_key(f.tid):
				thead = func_thread(f.tid)
				self.threads[f.tid] = thead
			self.threads[f.tid].funcs.append(f)
		infile.close()
		
	def process_maps(self , maps):
		for (k,v) in self.threads.items():
			for func in v.funcs:
				map = maps.find(func.addr)
				if map != None:
					func.libname = map.libname
					func.liboffset = func.addr - map.start
					self.process_funcname(func)
				else:
					print('can not find map for 0x%x'%(func.addr))
				
	def process_funcname(self , func):
		if self.funccache.has_key(func.addr):
			#print('has 0x%x,name:%s,info:%s'%(func.addr,self.funccache[func.addr].name,self.funccache[func.addr].info))
			func.info = self.funccache[func.addr].info
			func.name = self.funccache[func.addr].name
			#print(' 0x%x,name:%s,info:%s'%(func.addr,func.name,func.info))
		else:
			libfile = symbols_dir + func.libname;
			cmd = 'arm-linux-androideabi-addr2line '
			args = ' -a -i -p -s -f -C -e '
			command_all = cmd + hex(func.liboffset) + args + libfile
			(status , result) = commands.getstatusoutput(command_all)
			if status == 0:
				func.info = result
				func.name = result.split()[1]
				self.funccache[func.addr] = func
				#print('add 0x%x,name:%s'%(func.addr,func.name))
			else:
				print ('get funcname failed status:(%d),addr:0x%x,libname:%s'%(status,func.liboffset,func.libname))
		
	
	def convert_to_xml(self, output):
	
		doc = xml.dom.minidom.Document() 
		root = doc.createElement('root') 
		doc.appendChild(root) 
		
		for (k,v) in self.threads.items():
			thread_node = doc.createElement('thread') 
			thread_node.setAttribute('tid' , k)
			root.appendChild(thread_node)
			parent = thread_node
			func_depth = 0
			func_stack = []
			
			for func in v.funcs:
				if func.enter == 1:
					func_node = doc.createElement('function') 
					func_node.setAttribute('aaname' , func.name)
#					func_node.setAttribute('addr' , hex(func.addr))
#					func_node.setAttribute('lib' , func.libname)
#					func_node.setAttribute('info' , func.info)
					parent.appendChild(func_node)
					parent = func_node
					func_depth += 1
					func_stack.append(func)
				elif func.enter == 0:
					efunc = func_stack.pop()
					spendtime = str((func.sec - efunc.sec)*1000*1000 +  (func.nsec - efunc.nsec)/1000)
					parent.setAttribute('spend_us' , spendtime)
					parent = parent.parentNode
					func_depth -= 1
				else:
					func_node = doc.createElement('error') 
					parent.appendChild(func_node)
			while func_depth > 0 :
				parent.setAttribute('status' , 'not return')
				parent = parent.parentNode
				func_depth -= 1

		outfile = open(output , 'w')
		doc.writexml(outfile, indent='\t', addindent='\t', newl='\n', encoding="utf-8")
		outfile.close()
		

	
def main():
	maps = func_maps('maps.txt')
	trace = func_trace()
	trace.read('trace.txt')
	trace.process_maps(maps)
	
	
	
	trace.convert_to_xml('trace.xml')
	
main()

