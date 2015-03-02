from os import path
import shutil
import tempfile
from jpype import *

lucene_jar = path.join("..", "build", "lucene-1.4.3.jar")
if not path.isfile(lucene_jar):
    raise IOError, "Please provide %s" % path.abspath(lucene_jar)
startJVM(getDefaultJVMPath(),'-Djava.class.path=%s' % lucene_jar)

QueryParser = JClass("org.apache.lucene.queryParser.QueryParser")
IndexSearcher = JClass("org.apache.lucene.search.IndexSearcher")
IndexReader = JClass("org.apache.lucene.index.IndexReader")
StandardAnalyzer = JClass("org.apache.lucene.analysis.standard.StandardAnalyzer")
FSDirectory = JClass("org.apache.lucene.store.FSDirectory")
IndexWriter = JClass("org.apache.lucene.index.IndexWriter")
SimpleAnalyzer = JClass("org.apache.lucene.analysis.SimpleAnalyzer")

tmppath = tempfile.mkdtemp()
IndexWriter(tmppath, SimpleAnalyzer(), True).close()

directory = FSDirectory.getDirectory(tmppath, False)
reader = IndexReader.open(directory)
searcher = IndexSearcher(reader)
queryparser = QueryParser.parse("wenger","contents",StandardAnalyzer())
print queryparser.rewrite
print queryparser.rewrite.matchReport(reader)
qp = queryparser.rewrite(reader)
print qp
print searcher.search.matchReport(qp)
hits = searcher.search(qp)

shutil.rmtree(tmppath)
