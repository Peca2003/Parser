#include <iostream>
#include <fstream>
#include <string>
#include <curl/curl.h>
#include <libxml/HTMLparser.h>
#include <libxml/xpath.h>

class BookParser
{
public:
    BookParser()
    {
        curl_global_init(CURL_GLOBAL_DEFAULT);
        curl = curl_easy_init();
        if (!curl)
        {
            std::cerr << "Curl initialization failed." << std::endl;
        }
    }

    ~BookParser()
    {
        curl_easy_cleanup(curl);
        curl_global_cleanup();
    }

    void parseAndSaveBooks(const std::string& url, const std::string& outputFileName)
    {
        if (curl) {
            // Устанавливаем URL
            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

            // Устанавливаем User-Agent
            // Вместо "User-Agent" вставляем свой
            curl_easy_setopt(curl, CURLOPT_USERAGENT, "User-Agent");

            // Устанавливаем обработчик для записи данных
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

            // Выполняем запрос
            CURLcode res = curl_easy_perform(curl);

            // Проверяем наличие ошибок
            if (res != CURLE_OK)
            {
                std::cerr << "Curl failed: " << curl_easy_strerror(res) << std::endl;
                return;
            }

            // Парсим HTML с использованием libxml
            parseBooks();

            // Сохраняем данные в файл
            saveToFile(outputFileName);
        }
        else
        {
            std::cerr << "Curl initialization failed." << std::endl;
        }
    }

private:
    static size_t writeCallback(void* contents, size_t size, size_t nmemb, std::string* output)
    {
        size_t totalSize = size * nmemb;
        output->append(reinterpret_cast<char*>(contents), totalSize);
        return totalSize;
    }

    void parseBooks()
    {
        htmlDocPtr doc = htmlReadDoc(reinterpret_cast<const xmlChar*>(response.c_str()), nullptr, nullptr, HTML_PARSE_RECOVER | HTML_PARSE_NOERROR | HTML_PARSE_NOWARNING);
        if (doc)
        {
            // Создаем контекст XPath
            xmlXPathContextPtr xpathCtx = xmlXPathNewContext(doc);
            if (xpathCtx)
            {
                // Выполняем XPath запросы
                xpathQueryAndSave(xpathCtx, "//div[@class='product-title__head']", "//div[@class='product-title__author']", "//div[@class='product-price__value product-price__value--discount']", "//a[@class='product-card__picture product-card__row']");

                // Освобождаем контекст XPath
                xmlXPathFreeContext(xpathCtx);
            }

            // Освобождаем документ
            xmlFreeDoc(doc);
        }
    }

    void xpathQueryAndSave(xmlXPathContextPtr xpathCtx, const char* titleXpathExpr, const char* authorXpathExpr, const char* priceXpathExpr, const char* linkXpathExpr)
    {
        xmlXPathObjectPtr titleXpathObj = xmlXPathEvalExpression(reinterpret_cast<const xmlChar*>(titleXpathExpr), xpathCtx);
        xmlXPathObjectPtr authorXpathObj = xmlXPathEvalExpression(reinterpret_cast<const xmlChar*>(authorXpathExpr), xpathCtx);
        xmlXPathObjectPtr priceXpathObj = xmlXPathEvalExpression(reinterpret_cast<const xmlChar*>(priceXpathExpr), xpathCtx);
        xmlXPathObjectPtr linkXpathObj = xmlXPathEvalExpression(reinterpret_cast<const xmlChar*>(linkXpathExpr), xpathCtx);

        if (titleXpathObj && authorXpathObj && priceXpathObj && linkXpathObj)
        {
            std::ofstream outputFile("books_info.txt");

            for (int i = 0; i < titleXpathObj->nodesetval->nodeNr; ++i)
            {
                xmlNodePtr titleNode = titleXpathObj->nodesetval->nodeTab[i];
                xmlNodePtr authorNode = authorXpathObj->nodesetval->nodeTab[i];
                xmlNodePtr priceNode = priceXpathObj->nodesetval->nodeTab[i];
                xmlNodePtr linkNode = linkXpathObj->nodesetval->nodeTab[i];

                std::string titleContent = reinterpret_cast<const char*>(xmlNodeGetContent(titleNode));
                std::string authorContent = reinterpret_cast<const char*>(xmlNodeGetContent(authorNode));
                std::string priceContent = reinterpret_cast<const char*>(xmlNodeGetContent(priceNode));
                std::string linkContent = reinterpret_cast<const char*>(xmlNodeGetContent(linkNode));

                outputFile << "Title:" << titleContent << "Author:" << authorContent << "Price:" << priceContent << "Link:" << linkContent << "\n\n";
            }

            outputFile.close();

            // Освобождаем объекты XPath
            xmlXPathFreeObject(titleXpathObj);
            xmlXPathFreeObject(authorXpathObj);
            xmlXPathFreeObject(priceXpathObj);
            xmlXPathFreeObject(linkXpathObj);
        }
    }

    void saveToFile(const std::string& outputFileName)
    {
        std::ofstream outputFile(outputFileName);
        outputFile << response;
        outputFile.close();
    }

    CURL* curl;
    std::string response;
};

int main()
{
    BookParser bookParser;
    bookParser.parseAndSaveBooks("https://www.chitai-gorod.ru/catalog/books-18030/manga-110064", "site_code.txt");

    return 0;
}
