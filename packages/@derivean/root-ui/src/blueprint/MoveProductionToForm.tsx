/** @format */

import { BlueprintIcon } from "@derivean/ui";
import { zodResolver } from "@hookform/resolvers/zod";
import { Button, FormCss, FormError, FormInput, onSubmit, Tx, type Form } from "@use-pico/client";
import { type FC } from "react";
import { Controller, useForm } from "react-hook-form";
import { z } from "zod";
import { BlueprintPopupSelect } from "./PopupSelect";
import type { BlueprintTable } from "./Table";

const ShapeSchema = z.object({ blueprintId: z.string().min(1) });
type ShapeSchema = typeof ShapeSchema;

export namespace MoveProductionToForm {
	export interface Props extends Form.Props<ShapeSchema> {
		blueprintTableContext: BlueprintTable.Context;
	}
}

export const MoveProductionToForm: FC<MoveProductionToForm.Props> = ({
	blueprintTableContext,
	mutation,
	defaultValues,
	variant,
	tva = FormCss,
	css,
}) => {
	const form = useForm<z.infer<ShapeSchema>>({
		resolver: zodResolver(ShapeSchema),
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
				name={"blueprintId"}
				label={<Tx label={"Target blueprint (label)"} />}
			>
				<Controller
					control={form.control}
					name={"blueprintId"}
					render={({ field: { ref: _, ...field } }) => {
						return (
							<BlueprintPopupSelect
								context={blueprintTableContext}
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
