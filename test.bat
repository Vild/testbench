@echo off
set PATH="C:\VulkanSDK\1.1.70.1;%PATH%"
cd x64

Release\gl_testbench.exe gl45
Release\gl_testbench.exe gl45
Release\gl_testbench.exe gl45

Release\gl_testbench.exe vulkan
Release\gl_testbench.exe vulkan
Release\gl_testbench.exe vulkan
