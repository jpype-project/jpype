from jpype import * 
startJVM(getDefaultJVMPath(),'-Djava.class.path=c:/tools/lucene-1.4.3/lucene-1.4.3.jar') 

QueryParser = JClass("org.apache.lucene.queryParser.QueryParser") 
IndexSearcher = JClass("org.apache.lucene.search.IndexSearcher") 
IndexReader = JClass("org.apache.lucene.index.IndexReader") 
StandardAnalyzer = JClass("org.apache.lucene.analysis.standard.StandardAnalyzer") 
FSDirectory = JClass("org.apache.lucene.store.FSDirectory") 
IndexWriter = JClass("org.apache.lucene.index.IndexWriter")
SimpleAnalyzer = JClass("org.apache.lucene.analysis.SimpleAnalyzer")
 
IndexWriter('c:/temp/lucene', SimpleAnalyzer(), True).close() 
 
directory = FSDirectory.getDirectory("c:/temp/lucene",False) 
reader = IndexReader.open(directory) 
searcher = IndexSearcher(reader) 
queryparser = QueryParser.parse("wenger","contents",StandardAnalyzer()) 
print queryparser.rewrite
print queryparser.rewrite.matchReport(reader)
qp = queryparser.rewrite(reader) 
print qp
print searcher.search.matchReport(qp) 
hits = searcher.search(qp) 