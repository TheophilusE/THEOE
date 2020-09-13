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
  end
  dir = build_tree
  next if ENV['PLATFORM'] == 'android' || (Dir.exist?("#{dir}") and not ARGV.include?('cmake'))
  script = "script/cmake_#{ENV['GENERATOR']}#{ENV['OS'] ? '.bat' : '.sh'}"
  build_options = /linux|macOS|win/ =~ ENV['PLATFORM'] ? '' : "-D #{ENV['PLATFORM'].upcase}=1"
  File.readlines('script/.build-options').each { |var|
    var.chomp!
    build_options = "#{build_options} -D #{var}=#{ENV[var]}" if ENV[var]
  }
  system %Q{#{script} "#{dir}" #{build_options}} or abort
end

desc 'Clean the build tree'
task :clean do
  if ENV['PLATFORM'] == 'android'
    Rake::Task['gradle'].invoke('clean')
    next
  end
  system %Q{cmake --build "#{build_tree}" #{build_config} --target clean} or abort
end

desc 'Build the software'
task build: [:cmake] do
  system "ccache -z" if ENV['USE_CCACHE']
  if ENV['PLATFORM'] == 'android'
    Rake::Task['gradle'].invoke('build -x test')
    system "ccache -s" if ENV['USE_CCACHE']
    next
  end
  target = ENV['TARGET'] ? "--target #{ENV['TARGET']}" : '' # Build all by default
  filter = ''
  case ENV['GENERATOR']
  when 'xcode'
    concurrent = '' # Assume xcodebuild will do the right things without the '-jobs'
    filter = '|xcpretty -c && exit ${PIPESTATUS[0]}' if system('xcpretty -v >/dev/null 2>&1')
  when 'vs'
    concurrent = '/maxCpuCount'
  else
    concurrent = "-j #{$max_jobs}"
    filter = "2>#{lint_err_file}" if ENV['URHO3D_LINT']
  end
  system %Q{cmake --build "#{build_tree}" #{build_config} #{target} -- #{concurrent} #{ENV['BUILD_PARAMS']} #{filter}} or abort
  system "ccache -s" if ENV['USE_CCACHE']
end

desc 'Test the software'
task :test do
  if ENV['PLATFORM'] == 'android'
    Rake::Task['gradle'].invoke('test')
    next
  elsif ENV['URHO3D_LINT']
    Rake::Task['lint'].invoke
    next
  end
  dir = build_tree
  wrapper = ENV['CI'] && ENV['PLATFORM'] == 'linux' ? 'xvfb-run' : ''
  test = /xcode|vs/ =~ ENV['GENERATOR'] ? 'RUN_TESTS' : 'test'
  system %Q{#{wrapper} cmake --build "#{dir}" #{build_config} --target #{test}} or abort
end


### Internal tasks ###

task :gradle, [:task] do |_, args|
  system "./gradlew #{args[:task]} #{ENV['CI'] ? '--console plain' : ''}" or abort
end

task :lint do
  lint_err = File.read(lint_err_file)
  puts lint_err
  # TODO: Tighten the check by failing the job later
  # abort 'Failed to pass linter checks' unless lint_err.empty?
  # puts 'Passed the linter checks'
end

### Internal methods ###

def build_host
  ENV['HOST'] || RUBY_PLATFORM
end

def build_tree
  init_default
  ENV['BUILD_TREE'] || "build/#{ENV['PLATFORM'].downcase}"
end

def build_config
  /xcode|vs/ =~ ENV['GENERATOR'] ? "--config #{ENV.fetch('CONFIG', 'Release')}" : ''
end

def init_default
  case build_host
  when /linux/
    $max_jobs = `grep -c processor /proc/cpuinfo`.chomp
    ENV['GENERATOR'] = 'generic' unless ENV['GENERATOR']
    ENV['PLATFORM'] = 'linux' unless ENV['PLATFORM']
  when /darwin|macOS/
    $max_jobs = `sysctl -n hw.logicalcpu`.chomp
    ENV['GENERATOR'] = 'xcode' unless ENV['GENERATOR']
    ENV['PLATFORM'] = 'macOS' unless ENV['PLATFORM']
  when /win32|mingw|mswin|windows/
    require 'win32ole'
    WIN32OLE.connect('winmgmts://').ExecQuery("select NumberOfLogicalProcessors from Win32_ComputerSystem").each { |it|
      $max_jobs = it.NumberOfLogicalProcessors
    }
    ENV['GENERATOR'] = 'vs' unless ENV['GENERATOR']
    ENV['PLATFORM'] = 'win' unless ENV['PLATFORM']
  else
    abort "Unsupported host system: #{build_host}"
  end
end

def lint_err_file
  'build/clang-tidy.out'
end


# Load custom rake scripts
Dir['.github/workflows/*.rake'].each { |r| load r }
Dir['.rake/*.rake'].each { |r| load r }

# vi: set ts=2 sw=2 expandtab:
