#
# Copyright (C) 2016  Andy Walls <awalls.cx18@gmail.com>
#
# This file was automatically generated by gr_modtool from GNU Radio
#
# This file was automatically generated from a template incorporating
# data input by Andy Walls.
# See http://www.gnu.org/licenses/gpl-faq.en.html#GPLOutput .
#

# The presence of this file turns this directory into a Python package

'''
This is the NWR OOT GNU Radio module. Place your Python package
description here (python/__init__.py).
'''

# import swig generated symbols into the nwr namespace
try:
	# this might fail if the module is python-only
	from nwr_swig import *
except ImportError:
	pass

# import any pure python here
#