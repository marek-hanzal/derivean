/** @format */

import { ResourceIcon } from "@derivean/ui";
import { zodResolver } from "@hookform/resolvers/zod";
import { Button, FormCss, FormError, FormInput, onSubmit, Tx, type Form } from "@use-pico/client";
import { type FC } from "react";
import { Controller, useForm } from "react-hook-form";
import { ResourcePopupSelect } from "~/app/root/ResourcePopupSelect";
import { BlueprintProductionResourceSchema } from "~/app/schema/BlueprintProductionResourceSchema";

export namespace BlueprintProductionResourceForm {
	export interface Props extends Form.Props<BlueprintProductionResourceSchema["shape"]> {
		//
	}
}

export const BlueprintProductionResourceForm: FC<BlueprintProductionResourceForm.Props> = ({
	mutation,
	defaultValues,
	variant,
	tva = FormCss,
	css,
}) => {
	const form = useForm<BlueprintProductionResourceSchema["~shape"]>({
		resolver: zodResolver(BlueprintProductionResourceSchema.shape),
		defaultValues: { amount: 1, ...defaultValues },
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
