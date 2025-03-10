/** @format */

import type { serviceBlueprintGraph } from "@derivean/service";
import { Dependencies } from "@derivean/ui";
import { RequirementsInline } from "@derivean/ui/src/ui/RequirementsInline";
import type { BlueprintDependencySchema, BlueprintRequirementSchema } from "@derivean/utils";
import { Card as CoolCard, Tx } from "@use-pico/client";
import { toHumanNumber, tvc, type IdentitySchema } from "@use-pico/common";
import type { FC } from "react";
import { BlueprintDependenciesInline } from "./DependenciesInline";

namespace Card {
	export interface Data extends IdentitySchema.Type {
		name: string;
		cycles: number;
		requirements: (BlueprintRequirementSchema["~entity"] & { name: string })[];
		dependencies: (BlueprintDependencySchema["~entity"] & { name: string })[];
	}

	export interface Props extends CoolCard.PropsEx<Data> {
		dependencies: serviceBlueprintGraph.Result;
	}
}

const Card: FC<Card.Props> = ({ dependencies, ...props }) => {
	return (
		<CoolCard
			items={[
				{
					id: "name",
					label: <Tx label={"Building name (label)"} />,
					render({ entity }) {
						return entity.name;
					},
				},
				{
					id: "cycles",
					label: <Tx label={"Construction cycles (label)"} />,
					render({ entity }) {
						return toHumanNumber({ number: entity.cycles });
					},
				},
				{
					id: "requirements",
					label: <Tx label={"Required resources (label)"} />,
					render({ entity }) {
						return (
							<RequirementsInline
								requirements={entity.requirements}
								textEmpty={<Tx label={"No requirements (label)"} />}
							/>
						);
					},
				},
				{
					id: "dependencies",
					label: <Tx label={"Blueprint dependencies (label)"} />,
					render({ entity }) {
						return (
							<BlueprintDependenciesInline
								dependencies={entity.dependencies}
								textEmpty={<Tx label={"No dependencies (label)"} />}
							/>
						);
					},
				},
				{
					id: "deps",
					label: <Tx label={"Blueprint dependency graph (label)"} />,
					render({ entity }) {
						return (
							<Dependencies
								graph={dependencies}
								blueprintId={entity.id}
							/>
						);
					},
				},
				{
					id: "preview",
					render({ entity }) {
						return (
							<div
								className={tvc([
									"mx-auto",
									"border-2",
									"border-purple-400",
									"rounded-md",
									"w-[256px]",
									"h-[256px]",
									"bg-contain",
									`bg-${entity.id}`,
								])}
							/>
						);
					},
				},
			]}
			{...props}
		/>
	);
};

export { Card as BlueprintCard };
