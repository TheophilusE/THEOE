#
# Copyright (c) 2008-2020 the Urho3D project.
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#

task default: 'build'

desc 'Invoke CMake to configure and generate a build tree'
task :cmake do
  if ENV['CI']
    system 'cmake --version' or abort 'Failed to find CMake'
    puts
  end
  unless ENV['GENERATOR']
    case build_host
    when /linux/
      ENV['GENERATOR'] = 'generic'
    when /darwin|macOS/
      ENV['GENERATOR'] = 'xcode'
    when /win32|mingw|mswin|windows/
      ENV['GENERATOR'] = 'vs'
    else
      abort "Unsupported host system: #{build_host}"
    end
  end
  next if ENV['PLATFORM'] == 'android' || Dir.exist?("#{build_tree}")
  script = "script/cmake_#{ENV['GENERATOR']}#{ENV['OS'] ? '.bat' : '.sh'}"
  build_options = /linux|macOS|win/ =~ ENV['PLATFORM'] ? '' : "-D #{ENV['PLATFORM'].upcase}=1"
  File.readlines('script/.build-options').each { |var|
    var.chomp!
    build_options = "#{build_options} -D #{var}=#{ENV[var]}" if ENV[var]
  }
  system %Q{#{script} "#{build_tree}" #{build_options}} or abort
  puts
end

desc 'Clean the build tree'
task :clean do
  if ENV['PLATFORM'] == 'android'
    Rake::Task['gradle'].invoke('clean')
    next
  end
  system %Q{cmake --build "#{build_tree}" --target clean} or abort
end

desc 'Build the software'
task build: [:cmake] do
  if ENV['PLATFORM'] == 'android'
    Rake::Task['gradle'].invoke('build')
    next
  end
  config = /xcode|vs/ =~ ENV['GENERATOR'] ? "--config #{ENV.fetch('CONFIG', 'Release')}" : ''
  target = ENV['TARGET'] ? "--target #{ENV['TARGET']}" : ''
  case ENV['GENERATOR']
  when 'xcode'
    concurrent = '' # Assume xcodebuild will do the right things without the '-jobs'
  when 'vs'
    concurrent = '/maxCpuCount'
  else
    case build_host
    when /linux/
      $max_jobs = `grep -c processor /proc/cpuinfo`.chomp
    when /darwin|macOS/
      $max_jobs = `sysctl -n hw.logicalcpu`.chomp
    when /win32|mingw|mswin|windows/
      require 'win32ole'
      WIN32OLE.connect('winmgmts://').ExecQuery("select NumberOfLogicalProcessors from Win32_ComputerSystem").each { |it|
        $max_jobs = it.NumberOfLogicalProcessors
      }
    else
      abort "Unsupported host system: #{build_host}"
    end
    concurrent = "-j#{$max_jobs}"
  end
  system %Q{cmake --build "#{build_tree}" #{config} #{target} -- #{concurrent} #{ENV['BUILD_PARAMS']}} or abort
end


### Internal tasks ###

task :gradle, [:task] do |_, args|
  system "./gradlew #{args[:task]} #{ENV['CI'] ? '--console plain' : ''}" or abort
end


### Internal methods ###

def build_host
  ENV['HOST'] || RUBY_PLATFORM
end

def build_tree
  unless ENV['PLATFORM']
    case build_host
    when /linux/
      ENV['PLATFORM'] = 'linux'
    when /darwin|macOS/
      ENV['PLATFORM'] = 'macOS'
    when /win32|mingw|mswin|windows/
      ENV['PLATFORM'] = 'win'
    else
      abort "Unsupported host system: #{build_host}"
    end
  end
  ENV['BUILD_TREE'] || "build/#{ENV['PLATFORM'].downcase}"
end


# Load custom rake scripts
Dir['.github/workflows/*.rake'].each { |r| load r }
Dir['.rake/*.rake'].each { |r| load r }

# vi: set ts=2 sw=2 expandtab:
