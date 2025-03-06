/** @format */

import { BlueprintIcon } from "@derivean/ui";
import { BlueprintConflictSchema } from "@derivean/utils";
import { zodResolver } from "@hookform/resolvers/zod";
import { Button, FormCss, FormError, FormInput, onSubmit, Tx, type Form } from "@use-pico/client";
import { type FC } from "react";
import { Controller, useForm } from "react-hook-form";
import { BlueprintPopupSelect } from "./BlueprintPopupSelect";
import type { BlueprintTable } from "./BlueprintTable";

export namespace BlueprintConflictForm {
	export interface Props extends Form.Props<BlueprintConflictSchema["shape"]> {
		blueprintTableContext: BlueprintTable.Context;
	}
}

export const BlueprintConflictForm: FC<BlueprintConflictForm.Props> = ({
	blueprintTableContext,
	mutation,
	defaultValues,
	variant,
	tva = FormCss,
	css,
}) => {
	const form = useForm<BlueprintConflictSchema["~shape"]>({
		resolver: zodResolver(BlueprintConflictSchema.shape),
		defaultValues,
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
				name={"conflictId"}
				label={<Tx label={"Blueprint conflict (label)"} />}
				hint={<Tx label={"Blueprint conflict (hint)"} />}
			>
				<Controller
					control={form.control}
					name={"conflictId"}
					render={({ field: { ref: _, ...field } }) => {
						return (
							<BlueprintPopupSelect
								context={blueprintTableContext}
								textTitle={<Tx label={"Select blueprint conflict (title)"} />}
								allowEmpty
								{...field}
							/>
						);
					}}
				/>
			</FormInput>

			<div className={"flex flex-row justify-between gap-8"}>
				<Button
					iconEnabled={BlueprintIcon}
					type={"submit"}
				>
					<Tx label={"Save (submit)"} />
				</Button>
			</div>
		</form>
	);
};
