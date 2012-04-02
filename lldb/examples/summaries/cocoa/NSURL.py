"""
LLDB AppKit formatters

part of The LLVM Compiler Infrastructure
This file is distributed under the University of Illinois Open Source
License. See LICENSE.TXT for details.
"""
# summary provider for NSURL
import lldb
import ctypes
import objc_runtime
import metrics
import CFString
import Logger

statistics = metrics.Metrics()
statistics.add_metric('invalid_isa')
statistics.add_metric('invalid_pointer')
statistics.add_metric('unknown_class')
statistics.add_metric('code_notrun')

# despite the similary to synthetic children providers, these classes are not
# trying to provide anything but a summary for an NSURL, so they need not
# obey the interface specification for synthetic children providers
class NSURLKnown_SummaryProvider:
	def adjust_for_architecture(self):
		pass

	def __init__(self, valobj, params):
		logger = Logger.Logger()
		self.valobj = valobj;
		self.sys_params = params
		if not(self.sys_params.types_cache.NSString):
			self.sys_params.types_cache.NSString = self.valobj.GetTarget().FindFirstType('NSString').GetPointerType()
		if not(self.sys_params.types_cache.NSURL):
			self.sys_params.types_cache.NSURL = self.valobj.GetTarget().FindFirstType('NSURL').GetPointerType()
		self.update();

	def update(self):
		logger = Logger.Logger()
		self.adjust_for_architecture();

	# one pointer is the ISA
	# then there is one more pointer and 8 bytes of plain data
	# (which are also present on a 32-bit system)
	# plus another pointer, and then the real data
	def offset_text(self):
		logger = Logger.Logger()
		return 24 if self.sys_params.is_64_bit else 16
	def offset_base(self):
		logger = Logger.Logger()
		return self.offset_text()+self.sys_params.pointer_size

	def url_text(self):
		logger = Logger.Logger()
		text = self.valobj.CreateChildAtOffset("text",
							self.offset_text(),
							self.sys_params.types_cache.NSString)
		base = self.valobj.CreateChildAtOffset("base",
							self.offset_base(),
							self.sys_params.types_cache.NSURL)
		my_string = CFString.CFString_SummaryProvider(text,None)
		if len(my_string) > 0 and base.GetValueAsUnsigned(0) != 0:
			# remove final " from myself
			my_string = my_string[0:len(my_string)-1]
			my_string = my_string + ' -- '
			my_base_string = NSURL_SummaryProvider(base,None)
			if len(my_base_string) > 2:
				# remove @" marker from base URL string
				my_base_string = my_base_string[2:]
			my_string = my_string + my_base_string
		return my_string


class NSURLUnknown_SummaryProvider:
	def adjust_for_architecture(self):
		pass

	def __init__(self, valobj, params):
		logger = Logger.Logger()
		self.valobj = valobj;
		self.sys_params = params
		self.update()

	def update(self):
		logger = Logger.Logger()
		self.adjust_for_architecture();

	def url_text(self):
		logger = Logger.Logger()
		stream = lldb.SBStream()
		self.valobj.GetExpressionPath(stream)
		url_text_vo = self.valobj.CreateValueFromExpression("url","(NSString*)[" + stream.GetData() + " description]")
		if url_text_vo.IsValid():
			return CFString.CFString_SummaryProvider(url_text_vo,None)
		return '<variable is not NSURL>'


def GetSummary_Impl(valobj):
	logger = Logger.Logger()
	global statistics
	class_data,wrapper = objc_runtime.Utilities.prepare_class_detection(valobj,statistics)
	if wrapper:
		return wrapper
	
	name_string = class_data.class_name()
	logger >> "class name is: " + str(name_string)

	if name_string == 'NSURL':
		wrapper = NSURLKnown_SummaryProvider(valobj, class_data.sys_params)
		statistics.metric_hit('code_notrun',valobj)
	else:
		wrapper = NSURLUnknown_SummaryProvider(valobj, class_data.sys_params)
		statistics.metric_hit('unknown_class',valobj.GetName() + " seen as " + name_string)
	return wrapper;

def NSURL_SummaryProvider (valobj,dict):
	logger = Logger.Logger()
	provider = GetSummary_Impl(valobj);
	if provider != None:
		if isinstance(provider,objc_runtime.SpecialSituation_Description):
			return provider.message()
		try:
			summary = provider.url_text();
		except:
			summary = None
		logger >> "got summary " + str(summary)
		if summary == None or summary == '':
			summary = '<variable is not NSURL>'
		return summary
	return 'Summary Unavailable'

def __lldb_init_module(debugger,dict):
	debugger.HandleCommand("type summary add -F NSURL.NSURL_SummaryProvider NSURL CFURLRef")
