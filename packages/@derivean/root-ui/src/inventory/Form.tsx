/** @format */

import { InventoryIcon } from "@derivean/ui";
import { InventorySchema } from "@derivean/utils";
import { zodResolver } from "@hookform/resolvers/zod";
import {
	Button,
	FormCss,
	FormError,
	FormInput,
	onSubmit,
	Select,
	Tx,
	type Form,
} from "@use-pico/client";
import { translator } from "@use-pico/common";
import { type FC } from "react";
import { Controller, useForm } from "react-hook-form";
import { ResourcePopupSelect } from "../resource/PopupSelect";
import type { ResourceTable } from "../resource/Table";

export namespace InventoryForm {
	export interface Props extends Form.Props<InventorySchema["shape"]> {
		resourceTableContext: ResourceTable.Context;
	}
}

export const InventoryForm: FC<InventoryForm.Props> = ({
	resourceTableContext,
	mutation,
	defaultValues,
	variant,
	tva = FormCss,
	css,
}) => {
	const form = useForm<InventorySchema["~shape"]>({
		resolver: zodResolver(InventorySchema.shape),
		defaultValues: { amount: 0, limit: 0, type: "storage", ...defaultValues },
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
				label={<Tx label={"Resource name (label)"} />}
			>
				<Controller
					control={form.control}
					name={"resourceId"}
					render={({ field: { ref: _, ...field } }) => {
						return (
							<ResourcePopupSelect
								context={resourceTableContext}
								textTitle={<Tx label={"Select resource (title)"} />}
								allowEmpty
								{...field}
							/>
						);
					}}
				/>
			</FormInput>

			<FormInput
				formState={form.formState}
				name={"type"}
				label={<Tx label={"Inventory type (label)"} />}
			>
				<Controller
					control={form.control}
					name={"type"}
					render={({ field: { ref: _, ...field } }) => {
						return (
							<Select<{ id: string; value: string }>
								items={[
									{
										id: "storage",
										value: translator.text("Inventory type - storage"),
									},
									{
										id: "construction",
										value: translator.text("Inventory type - construction"),
									},
								]}
								render={({ entity }) => entity.value}
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
				name={"limit"}
				label={<Tx label={"Inventory limit (label)"} />}
			>
				<input
					type={"number"}
					className={tv.input()}
					{...form.register("limit")}
				/>
			</FormInput>

			<div className={"flex flex-row justify-between gap-8"}>
				<Button
					iconEnabled={InventoryIcon}
					type={"submit"}
				>
					<Tx label={"Save (submit)"} />
				</Button>
			</div>
		</form>
	);
};
