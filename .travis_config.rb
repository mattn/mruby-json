MRuby::Build.new do |conf|
  toolchain :gcc
  enable_debug

  gem :core => 'mruby-print'
  gem File.expand_path(File.dirname(__FILE__))
end
