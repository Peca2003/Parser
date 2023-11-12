#include <iostream>
#include <fstream>
#include <string>
#include <curl/curl.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>

class BookParser
{
public:
    static std::size_t WriteCallback(void* contents, std::size_t size, std::size_t nmemb, std::string* output)
    {
        std::size_t total_size = size * nmemb;
        output->append((char*)contents, total_size);
        return total_size;
    }

    static void ParseHTML(const std::string& html)
    {
        xmlDocPtr doc = xmlReadDoc(BAD_CAST html.c_str(), NULL, NULL, XML_PARSE_RECOVER | XML_PARSE_NOERROR | XML_PARSE_NOWARNING);
        if (doc == NULL)
        {
            std::cerr << "Error parsing HTML" << std::endl;
            return;
        }

        // XPath для извлечения информации о книге
        xmlXPathContextPtr context = xmlXPathNewContext(doc);
        xmlXPathObjectPtr titleNodes = xmlXPathEvalExpression(BAD_CAST "//div[@class='product-title__head']", context);
        xmlXPathObjectPtr authorNodes = xmlXPathEvalExpression(BAD_CAST "//div[@class='product-title__author']", context);
        xmlXPathObjectPtr priceNodes = xmlXPathEvalExpression(BAD_CAST "//div[@class='product-price__value product-price__value--discount']", context);
        xmlXPathObjectPtr linkNodes = xmlXPathEvalExpression(BAD_CAST "//a[@class='product-card__picture product-card__row']", context);

        // Обработка результатов XPath
        if (titleNodes && titleNodes->nodesetval && titleNodes->nodesetval->nodeNr > 0)
        {
            xmlNodePtr titleNode = titleNodes->nodesetval->nodeTab[0];
            std::string title = reinterpret_cast<const char*>(xmlNodeGetContent(titleNode));
            std::cout << "Title: " << title << std::endl;
        }

        if (authorNodes && authorNodes->nodesetval && authorNodes->nodesetval->nodeNr > 0)
        {
            xmlNodePtr authorNode = authorNodes->nodesetval->nodeTab[0];
            std::string author = reinterpret_cast<const char*>(xmlNodeGetContent(authorNode));
            std::cout << "Author: " << author << std::endl;
        }

        if (priceNodes && priceNodes->nodesetval && priceNodes->nodesetval->nodeNr > 0) {
            xmlNodePtr priceNode = priceNodes->nodesetval->nodeTab[0];
            std::string price = reinterpret_cast<const char*>(xmlNodeGetContent(priceNode));
            std::cout << "Price: " << price << std::endl;
        }

        if (linkNodes && linkNodes->nodesetval && linkNodes->nodesetval->nodeNr > 0)
        {
            xmlNodePtr linkNode = linkNodes->nodesetval->nodeTab[0];
            std::string link = reinterpret_cast<const char*>(xmlGetProp(linkNode, BAD_CAST "href"));
            std::cout << "Link: " << link << std::endl;
        }

        // Очистка ресурсов
        xmlXPathFreeObject(titleNodes);
        xmlXPathFreeObject(authorNodes);
        xmlXPathFreeObject(priceNodes);
        xmlXPathFreeObject(linkNodes);
        xmlXPathFreeContext(context);
        xmlFreeDoc(doc);
    }

    static void SaveToFile(const std::string& filename, const std::string& data)
    {
        std::ofstream outfile(filename);
        if (outfile.is_open())
        {
            outfile << data;
            outfile.close();
            std::cout << "Data saved to file: " << filename << std::endl;
        }
        else
        {
            std::cerr << "Unable to open file for writing: " << filename << std::endl;
        }
    }

    static void ParseAndSave(const std::string& url, const std::string& filename)
    {
        CURL* curl = curl_easy_init();
        if (curl)
        {
            std::string html;

            // Настройка параметров запроса
            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &html);

            // Выполнение запроса
            CURLcode res = curl_easy_perform(curl);

            // Проверка наличия ошибок
            if (res != CURLE_OK)
            {
                std::cerr << "Failed to fetch URL: " << curl_easy_strerror(res) << std::endl;
            }
            else
            {
                // Парсинг HTML и извлечение информации о книге
                ParseHTML(html);

                // Сохранение данных в файл
                SaveToFile(filename, html);
            }

            // Освобождение ресурсов CURL
            curl_easy_cleanup(curl);
        }
        else
        {
            std::cerr << "Failed to initialize CURL" << std::endl;
        }
    }
};

int main()
{
    std::string url = "https://www.chitai-gorod.ru/catalog/books-18030/manga-110064";
    std::string filename = "books.html";

    BookParser::ParseAndSave(url, filename);

    return 0;
}
