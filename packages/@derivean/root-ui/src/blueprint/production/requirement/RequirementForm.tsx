/** @format */

import { ResourceIcon } from "@derivean/ui";
import { BlueprintProductionRequirementSchema } from "@derivean/utils";
import { zodResolver } from "@hookform/resolvers/zod";
import {
	BoolInput,
	Button,
	FormCss,
	FormError,
	FormInput,
	onSubmit,
	Tx,
	type Form,
} from "@use-pico/client";
import { type FC } from "react";
import { Controller, useForm } from "react-hook-form";
import { ResourcePopupSelect } from "../../../resource/ResourcePopupSelect";
import type { ResourceTable } from "../../../resource/ResourceTable";

export namespace RequirementForm {
	export interface Props extends Form.Props<BlueprintProductionRequirementSchema["shape"]> {
		resourceTableContext: ResourceTable.Context;
	}
}

export const RequirementForm: FC<RequirementForm.Props> = ({
	resourceTableContext,
	mutation,
	defaultValues,
	variant,
	tva = FormCss,
	css,
}) => {
	const form = useForm<BlueprintProductionRequirementSchema["~shape"]>({
		resolver: zodResolver(BlueprintProductionRequirementSchema.shape),
		defaultValues: { amount: 1, passive: false, ...defaultValues },
	});

	const tv = tva({
		...variant,
		isLoading: form.formState.isLoading,
		isSubmitting: form.formState.isSubmitting,
		css,
	}).slots;

	return (
		<form
			className={tv.base()}
			onSubmit={onSubmit({ form, mutation })}
		>
			<FormError
				variant={{ highlight: true }}
				error={form.formState.errors.root}
			/>

			<FormInput
				formState={form.formState}
				name={"resourceId"}
				label={<Tx label={"Requirement (label)"} />}
			>
				<Controller
					control={form.control}
					name={"resourceId"}
					render={({ field: { ref: _, ...field } }) => {
						return (
							<ResourcePopupSelect
								context={resourceTableContext}
								allowEmpty
								{...field}
							/>
						);
					}}
				/>
			</FormInput>

			<FormInput
				formState={form.formState}
				name={"amount"}
				label={<Tx label={"Amount (label)"} />}
			>
				<input
					type={"number"}
					className={tv.input()}
					{...form.register("amount")}
				/>
			</FormInput>

			<FormInput
				formState={form.formState}
				name={"passive"}
				label={<Tx label={"Passive requirement (label)"} />}
				hint={<Tx label={"Passive requirement (hint)"} />}
			>
				<Controller
					control={form.control}
					name={"passive"}
					render={({ field: { ref: _, ...field } }) => {
						return <BoolInput {...field} />;
					}}
				/>
			</FormInput>

			<div className={"flex flex-row justify-between gap-8"}>
				<Button
					iconEnabled={ResourceIcon}
					type={"submit"}
				>
					<Tx label={"Save (submit)"} />
				</Button>
			</div>
		</form>
	);
};

export { RequirementForm as BlueprintProductionRequirementForm };
