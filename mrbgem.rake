MRuby::Gem::Specification.new('mruby-json') do |spec|
  spec.license = 'MIT'
  spec.authors = 'mattn'
  require 'open3'
  def run_command env, command
    STDOUT.sync = true
    Open3.popen2e(env, command) do |stdin, stdout, thread|
      print stdout.read
      fail "#{command} failed" if thread.value != 0
    end
  end
  Dir.chdir(spec.dir) do
    cmd = 'patch -Np1 --silent -i patch/parson.patch'
    run_command ENV, "if #{cmd} --dry-run; then #{cmd}; fi"
  end
end
