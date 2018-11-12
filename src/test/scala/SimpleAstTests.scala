import org.scalatest.FlatSpec
import scala.collection.mutable.ListBuffer

import sepia._

trait CompilerInstance extends ScheduleCompiler
											 with PipelineForCompiler with DslExp
											 with AstOps {
	self =>
    val codegen = new DslGenC {
      val IR: self.type = self
    }

	def ev(boundsGraph: Map[Int, Map[Int, Map[String, Bound]]])(in: Rep[Array[Array[Int]]]) = {
		prog(in)
		evalSched(sched, boundsGraph)
	}

	def compile(boundsGraph: Map[Int, Map[Int, Map[String, Bound]]]) = {
		codegen.emitSource(ev(boundsGraph), "pipeline",
			new java.io.PrintWriter(System.out))
	}
}

class CompilerSpec extends FlatSpec {
	"The grad program" should "return the default tree" in {
	 	val gradProg =
	 		new GradProg with CompilerInstance with TestAstOps

		val gradProgAnalysis = new GradProg with TestPipelineAnalysis

	 	gradProg.compile(gradProgAnalysis.getBoundsGraph)
	 	val correctAst: ScheduleNode[String, String] =
	 		new RootNode(List(
	 			new StorageNode("f",List(
	 				new LoopNode("y", "f", Sequential, List(
	 					new LoopNode("x", "f", Sequential, List(
	 						new ComputeNode("f", List())
	 					))
	 				))
	 			))
	 		))
			assertResult(correctAst)(gradProg.scheduleRep)
	}

	"The blurred grad program" should "return the default tree, with f inlined" in {
		val gradProg =
			new BlurredGradProg with CompilerInstance with TestAstOps

		val gradProgAnalysis = new BlurredGradProg with TestPipelineAnalysis

		gradProg.compile(gradProgAnalysis.getBoundsGraph)
		val correctAst: ScheduleNode[String, String] =
			new RootNode(List(
				new StorageNode("g",List(
					new LoopNode("y", "g", Sequential, List(
						new LoopNode("x", "g", Sequential, List(
							new ComputeNode("g", List())
						))
					))
				))
			))
			assertResult(correctAst)(gradProg.scheduleRep)
	}

	"The blurred grad prog with computeAt" should "deinline f and move it" in {
		val gradProg =
			new BlurredGradProgComputeAt with CompilerInstance with TestAstOps
		val gradProgAnalysis = new BlurredGradProgComputeAt with TestPipelineAnalysis
		gradProg.compile(gradProgAnalysis.getBoundsGraph)

		val correctAst: ScheduleNode[String, String] =
			new RootNode(List(
				new StorageNode("g",List(
					new LoopNode("y", "g", Sequential, List(
						new StorageNode("f", List(
							new LoopNode("y", "f", Sequential, List(
								new LoopNode("x", "f", Sequential, List(
									new ComputeNode("f", List())
								))
							)),
							new LoopNode("x", "g", Sequential, List(
								new ComputeNode("g", List())
							))
						))
					))
				))
			))

			assertResult(correctAst)(gradProg.scheduleRep)
	}

	"The three stage box blur program" should "return the default tree, with f & g inlined" in {
		val gradProg =
			new ThreeStageBoxBlur with CompilerInstance with TestAstOps

		val gradProgAnalysis = new ThreeStageBoxBlur with TestPipelineAnalysis

		println("Three stage box blur:")
		gradProg.compile(gradProgAnalysis.getBoundsGraph)
		val correctAst: ScheduleNode[String, String] =
			new RootNode(List(
				new StorageNode("h",List(
					new LoopNode("y", "h", Sequential, List(
						new LoopNode("x", "h", Sequential, List(
							new ComputeNode("h", List())
						))
					))
				))
			))
			assertResult(correctAst)(gradProg.scheduleRep)
	}

	"The three stage blur with computeAt" should "inline g, but precompute f" in {
		println("Three stage box blur with computeAt:")
		val blurProg =
			new ThreeStageBoxBlurWithComputeAt with CompilerInstance with TestAstOps
		val blurProgAnalysis = new ThreeStageBoxBlurWithComputeAt with TestPipelineAnalysis
		blurProg.compile(blurProgAnalysis.getBoundsGraph)

		val correctAst: ScheduleNode[String, String] =
			new RootNode(List(
				new StorageNode("h",List(
					new LoopNode("y", "h", Sequential, List(
						new StorageNode("f", List(
							new LoopNode("y", "f", Sequential, List(
								new LoopNode("x", "f", Sequential, List(
									new ComputeNode("f", List())
								))
							)),
							new LoopNode("x", "h", Sequential, List(
								new ComputeNode("h", List())
							))
						))
					))
				))
			))

		assertResult(correctAst)(blurProg.scheduleRep)
	}
}
