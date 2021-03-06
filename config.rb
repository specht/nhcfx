#!/usr/bin/env ruby

require 'fileutils'
require 'json'
require 'yaml'

# :static
# - :dynamic
#   - :neo4j
PROFILE = [:static, :dynamic]

# to get development mode, add the following to your ~/.bashrc:
# export QTS_DEVELOPMENT=1

STAGING = File::dirname(File::expand_path(__FILE__)).include?('staging')
DEVELOPMENT    = !(ENV['QTS_DEVELOPMENT'].nil?)
PROJECT_NAME = 'nhcfx' + (STAGING ? 'staging' : '') + (DEVELOPMENT ? 'dev' : '')
DEV_NGINX_PORT = 8020
DEV_NEO4J_PORT = 8021
LOGS_PATH = DEVELOPMENT ? './logs' : "/home/qts/logs/#{PROJECT_NAME}"
DATA_PATH = DEVELOPMENT ? './data' : "/home/qts/data/#{PROJECT_NAME}"
NEO4J_DATA_PATH = File::join(DATA_PATH, 'neo4j')
CACHE_FILES_PATH = File::join(DATA_PATH, 'cache')
BIN_FILES_PATH = File::join(DATA_PATH, 'bin')

docker_compose = {
    :version => '3',
    :services => {},
#     :networks => {:default => {:external => {:name => 'default'}}}
}

if PROFILE.include?(:static)
    docker_compose[:services][:nginx] = {
        :build => './docker/nginx',
        :volumes => [
            './src/static:/usr/share/nginx/html:ro',
            "#{CACHE_FILES_PATH}:/cache:ro",
            "#{BIN_FILES_PATH}:/app_bin:ro",
            "#{LOGS_PATH}:/var/log/nginx",
        ]
    }
    if !DEVELOPMENT
        if !STAGING
            docker_compose[:services][:nginx][:environment] = [
                'VIRTUAL_HOST=fx.nhcham.org',
                'LETSENCRYPT_HOST=fx.nhcham.org',
                'LETSENCRYPT_EMAIL=specht@quelltextschmiede.de'
            ]
        else
            docker_compose[:services][:nginx][:environment] = [
                'VIRTUAL_HOST=fx.nhcham.org',
                'LETSENCRYPT_HOST=fx.nhcham.org',
                'LETSENCRYPT_EMAIL=specht@quelltextschmiede.de'
            ]
        end
        docker_compose[:services][:nginx][:expose] = ['80']
    end
    if PROFILE.include?(:dynamic)
        docker_compose[:services][:nginx][:links] = ["ruby:#{PROJECT_NAME}_ruby_1"]
    end
    nginx_config = <<~eos
        log_format custom '$http_x_forwarded_for - $remote_user [$time_local] "$request" '
                          '$status $body_bytes_sent "$http_referer" '
                          '"$http_user_agent" "$request_time"';

        server {
            listen 80;
            server_name localhost;
            client_max_body_size 8M;

            access_log /var/log/nginx/access.log custom;

            charset utf-8;

            location /cache {
                root /;
            }

            location /api {
                proxy_pass http://#{PROJECT_NAME}_ruby_1:9292;
            }

            location / {
                root /usr/share/nginx/html;
            }
                              
            auth_basic "HERE BE DRAGONS";
            auth_basic_user_file /app_bin/htpasswd_nhcfx;
        }

    eos
    File::open('docker/nginx/default.conf', 'w') do |f|
        f.write nginx_config
    end
    if PROFILE.include?(:dynamic)
        docker_compose[:services][:nginx][:depends_on] = [:ruby]
    end
end

if PROFILE.include?(:dynamic)
    env = []
    env << 'DEVELOPMENT=1' if DEVELOPMENT
    env << 'STAGING=1' if STAGING
    docker_compose[:services][:ruby] = {
        :build => './docker/ruby',
        :volumes => ['./src/ruby:/app:ro',
                     './src:/src:ro',
                     './src/static:/static:ro',
                     "#{CACHE_FILES_PATH}:/cache",
                     "#{BIN_FILES_PATH}:/app_bin"],
        :environment => env,
        :working_dir => '/app',
        :entrypoint =>  DEVELOPMENT ?
            'rerun -b --dir /app -s SIGKILL \'rackup --host 0.0.0.0\'' :
            'rackup --host 0.0.0.0'
    }
    if PROFILE.include?(:neo4j)
        docker_compose[:services][:ruby][:depends_on] ||= []
        docker_compose[:services][:ruby][:depends_on] << :neo4j
        docker_compose[:services][:ruby][:links] = ['neo4j:neo4j']
    end
end

if PROFILE.include?(:neo4j)
    docker_compose[:services][:neo4j] = {
        :build => './docker/neo4j',
        :volumes => ["#{NEO4J_DATA_PATH}:/neo4j_data"]
    }
end

docker_compose[:services].values.each do |x|
#     x[:networks] = [:qts]
    x[:network_mode] = 'default'
end

if DEVELOPMENT
    docker_compose[:services][:nginx][:ports] = ["127.0.0.1:#{DEV_NGINX_PORT}:80"]
    if PROFILE.include?(:neo4j)
        docker_compose[:services][:neo4j][:ports] = ["127.0.0.1:#{DEV_NEO4J_PORT}:7474"]
    end
end

unless DEVELOPMENT
    docker_compose[:services].values.each do |x|
        x[:restart] = :always
    end
end

File::open('docker-compose.yaml', 'w') do |f|
    f.puts "# NOTICE: don't edit this file directly, use config.rb instead!\n"
    f.write(JSON::parse(docker_compose.to_json).to_yaml)
end

FileUtils::mkpath(LOGS_PATH)
if PROFILE.include?(:dynamic)
    FileUtils::cp('src/ruby/Gemfile', 'docker/ruby/')
    FileUtils::mkpath(CACHE_FILES_PATH)
    FileUtils::mkpath(BIN_FILES_PATH)
end
if PROFILE.include?(:neo4j)
    FileUtils::mkpath(NEO4J_DATA_PATH)
end

# system("docker network create -d bridge qts 2> /dev/null")
system("docker-compose --project-name #{PROJECT_NAME} #{ARGV.join(' ')}")
