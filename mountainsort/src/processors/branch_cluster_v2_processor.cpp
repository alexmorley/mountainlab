#include "branch_cluster_v2_processor.h"
#include "branch_cluster_v2.h"

class branch_cluster_v2_ProcessorPrivate
{
public:
	branch_cluster_v2_Processor *q;
};

branch_cluster_v2_Processor::branch_cluster_v2_Processor() {
	d=new branch_cluster_v2_ProcessorPrivate;
	d->q=this;

	this->setName("branch_cluster_v2");
	this->setVersion("0.1");
	this->setInputFileParameters("raw","detect","adjacency_matrix");
	this->setOutputFileParameters("firings");
	this->setRequiredParameters("clip_size","min_shell_size","shell_increment","num_features");
	this->setRequiredParameters("detect_interval");
}

branch_cluster_v2_Processor::~branch_cluster_v2_Processor() {
	delete d;
}

bool branch_cluster_v2_Processor::check(const QMap<QString, QVariant> &params)
{
	if (!this->checkParameters(params)) return false;
	return true;
}

bool branch_cluster_v2_Processor::run(const QMap<QString, QVariant> &params)
{
	Branch_Cluster_V2_Opts opts;

	QString raw_path=params["raw"].toString();
	QString detect_path=params["detect"].toString();
	QString adjacency_matrix_path=params["adjacency_matrix"].toString();
	QString firings_path=params["firings"].toString();
	opts.clip_size=params["clip_size"].toInt();
	opts.min_shell_size=params["min_shell_size"].toInt();
	opts.shell_increment=params["shell_increment"].toDouble();
	opts.num_features=params["num_features"].toInt();
	opts.detect_interval=params["detect_interval"].toInt();

	return branch_cluster_v2(raw_path,detect_path,adjacency_matrix_path,firings_path,opts);
}

