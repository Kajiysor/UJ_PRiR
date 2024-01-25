require 'timeout'
require 'nokogiri'
require 'open-uri'
require 'sqlite3'

$headers = { "User-Agent" => "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/58.0.3029.110 Safari/537.3", "Accept" => "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8" }

def create_database
    db = SQLite3::Database.new "crawld_data.db"

    db.execute <<-SQL
        CREATE TABLE IF NOT EXISTS products (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            title TEXT,
            price REAL,
            url TEXT
        );
    SQL

    return db
end

def print_table_data(node_set)
    return if node_set.empty?
    node_set.search('tr').each do |row|
        label_cell, value_cell = row.search('td')
        label = label_cell.at('span').text
        value = value_cell.at('span').text
        puts "#{label}: #{value}"
    end
end

def get_details(url, db)
    html = Nokogiri::HTML(URI.open($url_base + url, $headers))
    expander_content = html.css('.a-section')
    specs = expander_content.css('.a-section.a-spacing-small.a-spacing-top-small')
    table_rows = specs.search('table')
    print_table_data(table_rows)
    puts "URL : #{$url_base + url}"
end

def crawl_products(url, db)
    page = Nokogiri::HTML(URI.open(url, $headers))

    page.css('.s-result-item').each do |product|
        [".a-size-base-plus", ".a-size-medium"].each do |title_class|
            link = product.css(".a-link-normal")[0]
            title = product.css("#{title_class}").text
            price = product.css('.a-price-whole').text.gsub(/[^0-9,]+/, "").gsub(",", ".").to_f
            if !price.zero? and !title.empty?
                puts "====================================================================="
                puts ""
                puts "Title: #{title}"
                puts "Price: #{price} PLN"
                get_details(link["href"], db)
                puts ""
                puts "====================================================================="
                puts ""
                db.execute("INSERT INTO products (title, price, url) VALUES (?, ?, ?)", [title, price, $url_base + link["href"]])
            end
        end
    end
end

def crawl_website(category, key_word)
    db = create_database

    puts "Start crawling #{$url}"

    timeout_duration = 60

    begin
        Timeout.timeout(timeout_duration) do
            crawl_products($url, db)
        end
    rescue Timeout::Error
        puts "Crawling process timed out after #{timeout_duration} seconds."
    end

    db.close
end

category = "electronics"
key_word = ["samsung", "galaxy", "s24"]
$url = "https://www.amazon.pl/s?k=#{key_word.join("+")}&i=#{category}"
$url_base = "https://www.amazon.pl"

crawl_website(category, key_word)
