# BSD 2-Clause License

#
# Copyright (c) 2015, 2018 CNRS
# Authors: Joseph Mirabel
# All rights reserved.

# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:

# * Redistributions of source code must retain the above copyright notice, this
#   list of conditions and the following disclaimer.

# * Redistributions in binary form must reproduce the above copyright notice,
#   this list of conditions and the following disclaimer in the documentation
#   and/or other materials provided with the distribution.

# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

IF (GEPETTO_VIEWER_CORBA_FOUND)
  INCLUDE(${GEPETTO_VIEWER_CORBA_CMAKE_PLUGIN})

  ADD_SUBDIRECTORY(hppmonitoringplugin)
  # Disable because header <gepetto/gui/omniorb/omniorbthread.hh> no more accessible.
  #IF (NOT HPP_MANIPULATION_CORBA_CLIENT_ONLY)
  #  ADD_SUBDIRECTORY(hppmanipulationcorbaplugin)
  #ENDIF (NOT HPP_MANIPULATION_CORBA_CLIENT_ONLY)
ENDIF (GEPETTO_VIEWER_CORBA_FOUND)
